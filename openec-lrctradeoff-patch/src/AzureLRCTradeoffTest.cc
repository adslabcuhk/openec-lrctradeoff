#include "common/Config.hh"
#include "ec/ECDAG.hh"
#include "ec/ECPolicy.hh"
#include "ec/ECBase.hh"
#include "ec/ECTask.hh"
#include "ec/AzureLRCTradeoff.hh"

#include <map>
#include <utility>

using namespace std;

void usage()
{
    printf("usage: ./AzureLRCTradeoff k l g eta pktbytes mode failed_id\n");
}

double getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1e+6 + (double)tv.tv_usec;
}

int main(int argc, char **argv)
{

    if (argc < 8)
    {
        usage();
        return 0;
    }

    int eck = atoi(argv[1]);
    int ecl = atoi(argv[2]);
    int ecg = atoi(argv[3]);
    int eceta = atoi(argv[4]);
    int ecn = eck + ecl + ecg;
    int pktsizeB = atoi(argv[5]);
    string mode = string(argv[6]);
    int failed_id = atoi(argv[7]);

    string ecid = "AT_" + to_string(ecn) + "_" + to_string(eck) + "_" + to_string(eceta);

    if (mode == "repair")
    {
        ecid = ecid + "_r";
    }
    else if (mode == "maintenance")
    {
        ecid = ecid + "_m";
    }

    string confpath = "conf/sysSetting.xml";
    Config *conf = new Config(confpath);

    ECPolicy *ecpolicy = conf->_ecPolicyMap[ecid];
    ECBase *ec = ecpolicy->createECClass();

    int n = ec->_n;
    int k = ec->_k;
    int w = ec->_w;

    int n_data_symbols = k * w;
    int n_code_symbols = (n - k) * w;

    // 0. prepare data buffers
    char **databuffers = (char **)calloc(n_data_symbols, sizeof(char *));
    for (int i = 0; i < n_data_symbols; i++)
    {
        databuffers[i] = (char *)calloc(pktsizeB, sizeof(char));
        char v = i;
        memset(databuffers[i], v, pktsizeB);
    }

    // 1. prepare code buffers
    char **codebuffers = (char **)calloc(n_code_symbols, sizeof(char *));
    for (int i = 0; i < n_code_symbols; i++)
    {
        codebuffers[i] = (char *)calloc(pktsizeB, sizeof(char));
        memset(codebuffers[i], 0, pktsizeB);
    }

    double initEncodeTime = 0, initDecodeTime = 0;
    double encodeTime = 0, decodeTime = 0;
    initEncodeTime -= getCurrentTime();

    ECDAG *encdag = ec->Encode();
    vector<ECTask *> encodetasks;
    unordered_map<int, char *> encodeBufMap;
    vector<int> toposeq = encdag->toposort();
    for (int i = 0; i < toposeq.size(); i++)
    {
        ECNode *curnode = encdag->getNode(toposeq[i]);
        curnode->parseForClient(encodetasks);
    }
    for (int i = 0; i < n_data_symbols; i++)
        encodeBufMap.insert(make_pair(i, databuffers[i]));
    for (int i = 0; i < n_code_symbols; i++)
        encodeBufMap.insert(make_pair(n_data_symbols + i, codebuffers[i]));
    initEncodeTime += getCurrentTime();

    vector<int> shortening_free_list; // free list for shortening

    encodeTime -= getCurrentTime();
    for (int taskid = 0; taskid < encodetasks.size(); taskid++)
    {
        ECTask *compute = encodetasks[taskid];
        compute->dump();

        vector<int> children = compute->getChildren();
        unordered_map<int, vector<int>> coefMap = compute->getCoefMap();
        int col = children.size();
        int row = coefMap.size();

        vector<int> targets;
        int *matrix = (int *)calloc(row * col, sizeof(int));
        char **data = (char **)calloc(col, sizeof(char *));
        char **code = (char **)calloc(row, sizeof(char *));
        for (int bufIdx = 0; bufIdx < children.size(); bufIdx++)
        {
            int child = children[bufIdx];

            // support shortening
            if (child >= n * w && encodeBufMap.find(child) == encodeBufMap.end())
            {
                shortening_free_list.push_back(child);
                char *slicebuf = (char *)calloc(pktsizeB, sizeof(char));
                encodeBufMap[child] = slicebuf;
            }

            data[bufIdx] = encodeBufMap[child];
        }
        int codeBufIdx = 0;
        for (auto it : coefMap)
        {
            int target = it.first;
            char *codebuf;
            if (encodeBufMap.find(target) == encodeBufMap.end())
            {
                codebuf = (char *)calloc(pktsizeB, sizeof(char));
                encodeBufMap.insert(make_pair(target, codebuf));
            }
            else
            {
                codebuf = encodeBufMap[target];
            }
            code[codeBufIdx] = codebuf;
            targets.push_back(target);
            vector<int> curcoef = it.second;
            for (int j = 0; j < col; j++)
            {
                matrix[codeBufIdx * col + j] = curcoef[j];
            }
            codeBufIdx++;
        }
        Computation::Multi(code, data, matrix, row, col, pktsizeB, "Isal");

        free(matrix);
        free(data);
        free(code);
    }

    printf("shortening_free_list: ");
    for (auto pkt_idx : shortening_free_list)
    {
        printf("%d ", pkt_idx);
    }
    printf("\n");

    // free buffers in shortening free list
    for (auto pkt_idx : shortening_free_list)
    {
        free(encodeBufMap[pkt_idx]);
    }
    shortening_free_list.clear();

    encodeTime += getCurrentTime();

    // debug encode
    for (int i = 0; i < n_data_symbols; i++)
    {
        char *curbuf = (char *)databuffers[i];
        cout << "dataidx = " << i << ", value = " << (int)curbuf[0] << endl;
    }
    for (int i = 0; i < n_code_symbols; i++)
    {
        char *curbuf = (char *)codebuffers[i];
        cout << "codeidx = " << n_data_symbols + i << ", value = " << (int)curbuf[0] << endl;
    }

    cout << "========================" << endl;

    // decode
    initDecodeTime -= getCurrentTime();

    ECPolicy *ecpolicy1 = conf->_ecPolicyMap[ecid];
    ECBase *ec1 = ecpolicy->createECClass();

    vector<int> failsymbols;
    failsymbols.push_back(failed_id);
    unordered_map<int, char *> repairbuf;

    for (int i = 0; i < failsymbols.size(); i++)
    {
        char *tmpbuf = (char *)calloc(pktsizeB, sizeof(char));
        repairbuf[failsymbols[i]] = tmpbuf;
    }

    // only put the blocks not in the corresponding rack
    // get failed group id
    int failed_gp_id = -1;
    vector<vector<int>> group;
    ec->Place(group);
    for (int gp_id = 0; gp_id < group.size(); gp_id++)
    {
        for (auto bid : group[gp_id])
        {
            if (bid == failed_id)
            {
                failed_gp_id = gp_id;
                break;
            }
        }

        if (failed_gp_id != -1)
        {
            break;
        }
    }

    vector<int> availsymbols;
    for (int i = 0; i < n * w; i++)
    {
        if (mode == "repair")
        {
            if (i != failed_id)
                availsymbols.push_back(i);
        }
        else if (mode == "maintenance")
        {
            if (i != failed_id)
            {
                // if we cannot find the block in the failed group
                if (find(group[failed_gp_id].begin(), group[failed_gp_id].end(), i) == group[failed_gp_id].end())
                {
                    availsymbols.push_back(i);
                }
            }
        }
    }

    cout << "fail symbols: ";
    for (int i = 0; i < failsymbols.size(); i++)
    {
        cout << failsymbols[i] << " ";
    }
    cout << endl;

    cout << "avail symbols:";
    for (int i = 0; i < availsymbols.size(); i++)
    {
        cout << availsymbols[i] << " ";
    }
    cout << endl;

    ECDAG *decdag = ec1->Decode(availsymbols, failsymbols);
    vector<ECTask *> decodetasks;
    unordered_map<int, char *> decodeBufMap;
    vector<int> dectoposeq = decdag->toposort();
    for (int i = 0; i < dectoposeq.size(); i++)
    {
        ECNode *curnode = decdag->getNode(dectoposeq[i]);
        curnode->parseForClient(decodetasks);
    }
    for (int i = 0; i < n_data_symbols; i++)
    {
        if (find(failsymbols.begin(), failsymbols.end(), i) == failsymbols.end())
            decodeBufMap.insert(make_pair(i, databuffers[i]));
        else
            decodeBufMap.insert(make_pair(i, repairbuf[i]));
    }
    for (int i = 0; i < n_code_symbols; i++)
        if (find(failsymbols.begin(), failsymbols.end(), n_data_symbols + i) == failsymbols.end())
            decodeBufMap.insert(make_pair(n_data_symbols + i, codebuffers[i]));
        else
            decodeBufMap.insert(make_pair(i, repairbuf[n_data_symbols + i]));

    initDecodeTime += getCurrentTime();

    decodeTime -= getCurrentTime();

    shortening_free_list.clear();

    /**
     * @brief record number of disk seeks and number of sub-packets to read for every node
     * @disk_read_pkts_map <node> sub-packets read in each node
     * @disk_read_info_map <node_id, <num_disk_seeks, num_pkts_read>>
     *
     */
    map<int, vector<int>> disk_read_pkts_map;
    map<int, pair<int, int>> disk_read_info_map;

    int sum_packets_read = 0;
    double norm_repair_bandwidth = 0;

    // init the map
    for (int node_id = 0; node_id < n; node_id++)
    {
        disk_read_pkts_map[node_id].clear();
        disk_read_info_map[node_id] = make_pair(0, 0);
    }

    for (int taskid = 0; taskid < decodetasks.size(); taskid++)
    {
        ECTask *compute = decodetasks[taskid];
        compute->dump();

        vector<int> children = compute->getChildren();
        unordered_map<int, vector<int>> coefMap = compute->getCoefMap();
        int col = children.size();
        int row = coefMap.size();

        vector<int> targets;
        int *matrix = (int *)calloc(row * col, sizeof(int));
        char **data = (char **)calloc(col, sizeof(char *));
        char **code = (char **)calloc(row, sizeof(char *));
        for (int bufIdx = 0; bufIdx < children.size(); bufIdx++)
        {
            int child = children[bufIdx];

            // support shortening
            if (child >= n * w && decodeBufMap.find(child) == decodeBufMap.end())
            {
                shortening_free_list.push_back(child);
                char *slicebuf = (char *)calloc(pktsizeB, sizeof(char));
                decodeBufMap[child] = slicebuf;
            }

            if (child < n * w)
            {
                int node_id = child / w;
                vector<int> &read_pkts = disk_read_pkts_map[node_id];
                if (find(read_pkts.begin(), read_pkts.end(), child) == read_pkts.end())
                {
                    read_pkts.push_back(child);
                }
            }

            data[bufIdx] = decodeBufMap[child];
        }
        int codeBufIdx = 0;
        for (auto it : coefMap)
        {
            int target = it.first;
            char *codebuf;
            if (decodeBufMap.find(target) == decodeBufMap.end())
            {
                codebuf = (char *)calloc(pktsizeB, sizeof(char));
                decodeBufMap.insert(make_pair(target, codebuf));
            }
            else
            {
                codebuf = decodeBufMap[target];
            }
            code[codeBufIdx] = codebuf;
            targets.push_back(target);
            vector<int> curcoef = it.second;
            for (int j = 0; j < col; j++)
            {
                matrix[codeBufIdx * col + j] = curcoef[j];
            }
            codeBufIdx++;
        }
        Computation::Multi(code, data, matrix, row, col, pktsizeB, "Isal");
        free(matrix);
        free(data);
        free(code);
    }

    // printf("shortening_free_list: ");
    // for (auto pkt_idx : shortening_free_list)
    // {
    //     printf("%d ", pkt_idx);
    // }
    // printf("\n");

    // free buffers in shortening free list
    for (auto pkt_idx : shortening_free_list)
    {
        free(decodeBufMap[pkt_idx]);
    }
    shortening_free_list.clear();

    decodeTime += getCurrentTime();

    /**
     * @brief record the disk_read_info_map
     */
    for (auto item : disk_read_pkts_map)
    {
        vector<int> &read_pkts = item.second;
        sort(read_pkts.begin(), read_pkts.end());
    }

    for (int node_id = 0; node_id < n; node_id++)
    {
        vector<int> &list = disk_read_pkts_map[node_id];

        // we first transfer items in list %w
        vector<int> offset_list;
        for (int i = 0; i < list.size(); i++)
        {
            offset_list.push_back(list[i] % w);
        }
        sort(offset_list.begin(), offset_list.end()); // sort in ascending order
        reverse(offset_list.begin(), offset_list.end());

        // create consecutive read list
        int num_of_cons_reads = 0;
        vector<int> cons_list;
        vector<vector<int>> cons_read_list; // consecutive read list
        while (offset_list.empty() == false)
        {
            int offset = offset_list.back();
            offset_list.pop_back();

            if (cons_list.empty() == true)
            {
                cons_list.push_back(offset);
            }
            else
            {
                // it's consecutive
                if (cons_list.back() + 1 == offset)
                {
                    cons_list.push_back(offset); // at to the back of prev cons_list
                }
                else
                {
                    cons_read_list.push_back(cons_list); // commits prev cons_list
                    cons_list.clear();
                    cons_list.push_back(offset); // at to the back of new cons_list
                }
            }
        }
        if (cons_list.empty() == false)
        {
            cons_read_list.push_back(cons_list);
        }

        // printf("node id: %d, cons_read_list:\n", node_id);
        // for (auto cons_list : cons_read_list)
        // {
        //     for (auto offset : cons_list)
        //     {
        //         printf("%d ", offset);
        //     }
        //     printf("\n");
        // }

        // update disk_read_info_map
        disk_read_info_map[node_id].first = cons_read_list.size();
        disk_read_info_map[node_id].second = disk_read_pkts_map[node_id].size();

        // update stats
        sum_packets_read += disk_read_pkts_map[node_id].size();
    }

    // calculate norm repair bandwidth (against RS code)
    norm_repair_bandwidth = sum_packets_read * 1.0 / (k * w);

    // printf("disk read info:\n");
    // for (int node_id = 0; node_id < n; node_id++)
    // {
    //     printf("node_id: %d, num_disk_seeks: %d, num_pkts_read: %d, pkts: ", node_id, disk_read_info_map[node_id].first, disk_read_info_map[node_id].second);
    //     // printf("%d, %d\n", disk_read_info_map[node_id].first, disk_read_info_map[node_id].second);
    //     for (auto pkt : disk_read_pkts_map[node_id])
    //     {
    //         printf("%d ", pkt);
    //     }
    //     printf("\n");
    // }
    printf("packets read: %d / %d, norm repair bandwidth: %f\n", sum_packets_read, k * w, norm_repair_bandwidth);

    // debug decode
    for (int i = 0; i < failsymbols.size(); i++)
    {
        int failidx = failsymbols[i];
        char *curbuf = decodeBufMap[failidx];
        cout << "failidx = " << failidx << ", value = " << (int)curbuf[0] << endl;

        int failed_node = failidx / w;

        int diff = 0;

        if (failed_node < k)
        {
            diff = memcmp(decodeBufMap[failidx], databuffers[failidx], pktsizeB * sizeof(char));
        }
        else
        {
            diff = memcmp(decodeBufMap[failidx], codebuffers[failidx - n_data_symbols], pktsizeB * sizeof(char));
        }
        if (diff != 0)
        {
            printf("failed to decode data!!!!\n");
        }
    }
}
