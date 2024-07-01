#include "AzureLRCTradeoff.hh"

AzureLRCTradeoff::AzureLRCTradeoff(int n, int k, int w, int opt, vector<string> param)
{
    _n = n;
    _k = k;
    _w = w;
    _opt = opt;
    if (param.size() != 4)
    {
        printf("AzureLRCTradeoff::error invalid params (l,g,eta,approach)\n");
    }

    // two parameters in <param>
    // 1. l (number of local parity blocks)
    // 2. g (number of local parity blocks)
    // 3. eta (tradeoff data placement parameter)
    // 4. approach (used in distributed mode only); 0: repair; 1: maintenance)
    _l = atoi(param[0].c_str());
    _g = atoi(param[1].c_str());
    _eta = atoi(param[2].c_str());
    _approach = atoi(param[3].c_str());

    _encode_matrix = (int *)malloc(_n * _k * sizeof(int));
    generateMatrix(_encode_matrix, _k, _l, _g, 8);

    printf("AzureLRCTradeoff::_encode_matrix:\n");
    for (int i = 0; i < _n; i++)
    {
        for (int j = 0; j < _k; j++)
        {
            printf("%d ", _encode_matrix[i * _k + j]);
        }
        printf("\n");
    }

    vector<vector<int>> group;
    Place(group);
    printf("groups:\n");
    for (auto item : group)
    {
        for (auto it : item)
        {
            printf("%d ", it);
        }
        printf("\n");
    }
}

ECDAG *AzureLRCTradeoff::Encode()
{
    ECDAG *ecdag = new ECDAG();
    vector<int> data;
    vector<int> code;
    for (int i = 0; i < _k; i++)
    {
        data.push_back(i);
    }

    for (int i = _k; i < _n; i++)
    {
        code.push_back(i);
    }

    for (int i = 0; i < code.size(); i++)
    {
        vector<int> coef;
        for (int j = 0; j < _k; j++)
        {
            coef.push_back(_encode_matrix[(i + _k) * _k + j]);
        }
        ecdag->Join(code[i], data, coef);
    }
    if (code.size() > 1)
    {
        ecdag->BindX(code);
    }

    return ecdag;
}

ECDAG *AzureLRCTradeoff::Decode(vector<int> from, vector<int> to)
{
    printf("from: ");
    for (auto item : from)
    {
        printf("%d ", item);
    }
    printf("; ");

    printf("to: ");
    for (auto item : to)
    {
        printf("%d ", item);
    }
    printf("\n");

    if (to.size() == 1)
    {
        if (_approach == 0)
        {
            return DecodeSingleRepair(from, to);
        }
        else if (_approach == 1)
        {
            // check if satisfy maintenance constraints
            if (checkMaintenanceConstraints(from, to) == true)
            {
                return DecodeMaintenance(from, to);
            }
            else
            {
                printf("AzureLRCTradeoff:: unsupported failure for maintenance\n");
                ECDAG *ecdag = new ECDAG();
                return ecdag;
            }
        }
        else
        {
            printf("AzureLRCTradeoff:: unsupported approach %d\n", _approach);
            ECDAG *ecdag = new ECDAG();
            return ecdag;
        }
    }
    else
    {
        printf("AzureLRCTradeoff:: multiple failures repair is not implemented\n");
        ECDAG *ecdag = new ECDAG();
        return ecdag;
    }
}

ECDAG *AzureLRCTradeoff::DecodeSingleRepair(vector<int> from, vector<int> to)
{
    printf("DecodeSingleRepair::decode with single block repair\n");

    ECDAG *ecdag = new ECDAG();

    // can recover by local parity
    int ridx = to[0];
    vector<int> data;
    vector<int> coef;
    int nr = _k / _l;

    if (ridx < _k)
    {
        // source data
        int gidx = ridx / nr;
        for (int i = 0; i < nr; i++)
        {
            int idxinstripe = gidx * nr + i;
            if (ridx != idxinstripe)
            {
                data.push_back(idxinstripe);
                coef.push_back(1);
            }
        }
        data.push_back(_k + gidx);
        coef.push_back(1);
    }
    else if (ridx < (_k + _l))
    {
        // local parity
        int gidx = ridx - _k;
        for (int i = 0; i < nr; i++)
        {
            int idxinstripe = gidx * nr + i;
            data.push_back(idxinstripe);
            coef.push_back(1);
        }
    }
    else
    {
        // global parity
        generateMatrix(_encode_matrix, _k, _l, _g, 8);
        for (int i = 0; i < _k; i++)
        {
            data.push_back(i);
            coef.push_back(_encode_matrix[ridx * _k + i]);
        }
    }
    ecdag->Join(ridx, data, coef);

    return ecdag;
}

ECDAG *AzureLRCTradeoff::DecodeMaintenance(vector<int> from, vector<int> to)
{
    printf("DecodeMaintenance::decode with maintenance\n");

    // can recover by local parity
    int failed_idx = to[0];
    int _b = _k / _l;

    if (failed_idx < _k)
    {
        // local group id
        int corres_lg_id = failed_idx / _b;

        // check if it satisfy local maintenance
        int failed_gp_id = getResidingGroup(failed_idx);
        vector<vector<int>> group;
        Place(group);

        bool is_local_maintenance = true;
        for (auto blk_id : group[failed_gp_id])
        {
            if (blk_id == failed_idx)
            {
                continue;
            }

            // check whether both blocks belong to the same local group
            if (blk_id < _k)
            { // data block
                int lg_id = blk_id / _b;
                if (lg_id == corres_lg_id)
                {
                    is_local_maintenance = false;
                    break;
                }
            }
            else if (blk_id < _k + _l)
            { // local parity block
                if (blk_id - _k == corres_lg_id)
                {
                    is_local_maintenance = false;
                    break;
                }
            }
        }
        if (is_local_maintenance == true)
        {
            return DecodeLocalMaintenance(from, to);
        }
        else
        {
            return DecodeGlobalMaintenance(from, to);
        }
    }
    else
    {
        printf("AzureLRCTradeoff:: maintenance for parity blocks is not implemented\n");

        ECDAG *ecdag = new ECDAG();
        return ecdag;
    }
}

ECDAG *AzureLRCTradeoff::DecodeLocalMaintenance(vector<int> from, vector<int> to)
{
    printf("DecodeLocalMaintenance::decode with local maintenance\n");

    // the blocks from the corresponding local group are all available, thus
    // we don't need to traverse the blocks in <from>

    ECDAG *ecdag = new ECDAG();

    // can recover by local parity
    int failed_blk = to[0];
    vector<int> data;
    vector<int> coef;
    int _b = _k / _l;

    if (failed_blk < _k)
    {
        int corres_lg_id = failed_blk / _b;
        // add data blocks
        for (int idx = 0; idx < _b; idx++)
        {
            int avail_blk_id = corres_lg_id * _b + idx;
            if (failed_blk != avail_blk_id)
            {
                data.push_back(avail_blk_id);
                coef.push_back(1);
            }
        }
        // add local parity block
        data.push_back(_k + corres_lg_id);
        coef.push_back(1);
    }

    ecdag->Join(failed_blk, data, coef);

    return ecdag;
}

ECDAG *AzureLRCTradeoff::DecodeGlobalMaintenance(vector<int> from, vector<int> to)
{
    printf("DecodeGlobalMaintenance::decode with global maintenance\n");

    // the blocks not residing in the failed rack are all available, thus
    // we don't need to traverse the blocks in <from>

    ECDAG *ecdag = new ECDAG();

    int failed_idx = to[0];
    int _b = _k / _l;
    int failed_gp_id = getResidingGroup(failed_idx);
    vector<vector<int>> group;
    Place(group);
    vector<int> &failed_group = group[failed_gp_id];

    // categorize available data blocks and failed data blocks
    vector<int> avail_dbs;
    vector<int> failed_dbs;
    for (int blk_id = 0; blk_id < _k; blk_id++)
    {
        if (find(failed_group.begin(), failed_group.end(), blk_id) != failed_group.end())
        {
            failed_dbs.push_back(blk_id);
        }
        else
        {
            avail_dbs.push_back(blk_id);
        }
    }

    // categorize the blocks by their corresponding local group
    vector<vector<int>> failed_rack_blk_lg;
    for (int lg_id = 0; lg_id < _l; lg_id++)
    {
        failed_rack_blk_lg.push_back(vector<int>());
    }

    for (auto blk_id : failed_group)
    {
        if (blk_id < _k)
        {
            int lg_id = blk_id / _b;
            failed_rack_blk_lg[lg_id].push_back(blk_id);
        }
        else if (blk_id < _k + _l)
        {
            int lg_id = blk_id - _k;
            failed_rack_blk_lg[lg_id].push_back(blk_id);
        }
    }

    // virtual symbols (_n, _n+1, ..., _n + x - 1), representing the failed
    // data blocks
    vector<int> vir_syms;
    vector<bool> is_vir_sym_lp; // check whether the virtual symbol corresponds to local parity block

    // number of used global parity blocks (for global maintenance)
    int num_used_gp = 0;

    // construct the encoding matrix for the virtual symbols
    // i.e., [mtx] * [failed_blocks_in_the_rack] = [virtual_symbols]
    int *rec_matrix = (int *)malloc(failed_dbs.size() * failed_dbs.size() * sizeof(int));
    memset(rec_matrix, 0, failed_dbs.size() * failed_dbs.size() * sizeof(int));
    int rec_mtx_rid = 0;

    for (int lg_id = 0; lg_id < _l; lg_id++)
    { // check each local group
        auto &lg = failed_rack_blk_lg[lg_id];
        if (lg.size() == 0)
        { // there is no block from this local group stored in the failed rack
            continue;
        }
        else if (lg.size() == 1)
        { // there is only one block from this local group stored in the failed rack
            int unavail_blk_id = lg[0];
            if (unavail_blk_id < _k)
            {
                // 1. construct recover matrix
                for (int cid = 0; cid < failed_dbs.size(); cid++)
                {
                    if (failed_dbs[cid] != unavail_blk_id)
                    {
                        rec_matrix[rec_mtx_rid * failed_dbs.size() + cid] = 1;
                    }
                }
                rec_mtx_rid++;

                // 2. update ECDAG
                // add to vir_syms
                int vir_sym = _n + vir_syms.size();
                vir_syms.push_back(vir_sym);
                is_vir_sym_lp.push_back(true);

                vector<int> data;
                vector<int> coef;

                // available data blocks
                for (int idx = 0; idx < _b; idx++)
                {
                    int blk_id = lg_id * _b + idx;
                    if (blk_id != unavail_blk_id)
                    {
                        data.push_back(blk_id);
                        coef.push_back(1);
                    }
                }
                // available local parity block
                data.push_back(_k + lg_id);
                coef.push_back(1);

                ecdag->Join(vir_sym, data, coef);
            }
        }
        else
        {
            // check whether the corresponding local parity block is available
            bool is_lp_available = true;
            if (find(lg.begin(), lg.end(), _k + lg_id) != lg.end())
            {
                is_lp_available = false;
            }

            // add lp.size() - 1 virtual symbols corresponding to global
            // parity blocks
            for (int vs_id = 0; vs_id < lg.size() - 1; vs_id++)
            {
                // used global parity block id
                int corres_gp_id = _k + _l + num_used_gp;

                // 1. construct recover matrix
                for (int cid = 0; cid < failed_dbs.size(); cid++)
                {
                    rec_matrix[rec_mtx_rid * failed_dbs.size() + cid] = _encode_matrix[corres_gp_id * _k + failed_dbs[cid]];
                }
                rec_mtx_rid++;

                // 2. update ECDAG
                // add to vir_syms
                int vir_sym = _n + vir_syms.size();
                vir_syms.push_back(vir_sym);
                is_vir_sym_lp.push_back(false);

                // data blocks
                vector<int> data;
                vector<int> coef;

                // available data blocks
                for (auto avail_blk_id : avail_dbs)
                {
                    data.push_back(avail_blk_id);
                    // corresponding entry in encoding matrix
                    coef.push_back(_encode_matrix[corres_gp_id * _k + avail_blk_id]);
                }

                // available global parity block
                data.push_back(corres_gp_id);
                coef.push_back(1);

                ecdag->Join(vir_sym, data, coef);

                // update the number of used global parity blocks
                num_used_gp++;
            }

            // add the corresponding local parity block
            if (is_lp_available == true)
            {
                // 1. construct recover matrix
                for (int cid = 0; cid < failed_dbs.size(); cid++)
                {
                    if (failed_dbs[cid] / _b == lg_id)
                    {
                        rec_matrix[rec_mtx_rid * failed_dbs.size() + cid] = 1;
                    }
                }
                rec_mtx_rid++;

                // 2. update ECDAG
                // add to vir_syms
                int vir_sym = _n + vir_syms.size();
                vir_syms.push_back(vir_sym);
                is_vir_sym_lp.push_back(false);

                // data blocks
                vector<int> data;
                vector<int> coef;

                // available data blocks
                for (auto avail_blk_id : avail_dbs)
                {
                    if (avail_blk_id / _b == lg_id)
                    {
                        data.push_back(avail_blk_id);
                        // corresponding entry in encoding matrix
                        coef.push_back(1);
                    }
                }

                data.push_back(_k + lg_id);
                coef.push_back(1);

                ecdag->Join(vir_sym, data, coef);
            }
        }
    }

    // check if the number of virtual symbols are correct
    if (vir_syms.size() != failed_dbs.size())
    {
        printf("AzureLRCTradeoff:: incorrect number of virtual symbols: %ld, %ld\n", failed_dbs.size(), vir_syms.size());
        return ecdag;
    }

    printf("AzureLRCTradeoff::rec_matrix:\n");
    for (int i = 0; i < failed_dbs.size(); i++)
    {
        for (int j = 0; j < failed_dbs.size(); j++)
        {
            printf("%d ", rec_matrix[i * failed_dbs.size() + j]);
        }
        printf("\n");
    }

    // invert rec_matrix
    // i.e., [inv_mtx] * [virtual_symbols] = [failed_blocks_in_the_rack]
    int *inv_rec_matrix = (int *)malloc(failed_dbs.size() * failed_dbs.size() * sizeof(int));
    jerasure_invert_matrix(rec_matrix, inv_rec_matrix, failed_dbs.size(), 8);

    printf("AzureLRCTradeoff::inv_rec_matrix:\n");
    for (int i = 0; i < failed_dbs.size(); i++)
    {
        for (int j = 0; j < failed_dbs.size(); j++)
        {
            printf("%d ", inv_rec_matrix[i * failed_dbs.size() + j]);
        }
        printf("\n");
    }

    for (int idx = 0; idx < failed_dbs.size(); idx++)
    {
        if (failed_dbs[idx] == failed_idx)
        {
            vector<int> data;
            vector<int> coef;
            for (int cid = 0; cid < failed_dbs.size(); cid++)
            {
                data.push_back(vir_syms[cid]);
                coef.push_back(inv_rec_matrix[idx * failed_dbs.size() + cid]);
            }

            ecdag->Join(failed_idx, data, coef);
        }
    }

    return ecdag;
}

bool AzureLRCTradeoff::checkMaintenanceConstraints(vector<int> from, vector<int> to)
{
    int failed_blk_id = to[0];

    // only support maintenance of data blocks
    if (failed_blk_id >= _k)
    {
        return false;
    }

    // get failed group id
    vector<vector<int>> group;
    Place(group);
    int failed_gp_id = getResidingGroup(failed_blk_id);

    // identify whether all blocks are failed in group <failed_gp_id>
    bool ret_val = true;
    for (auto fb_id : group[failed_gp_id])
    {
        // if we can identify the block is alive
        if (find(from.begin(), from.end(), fb_id) != from.end())
        {
            ret_val = false;
            break;
        }
    }

    return ret_val;
}

int AzureLRCTradeoff::getResidingGroup(int blk_id)
{
    int reside_gp_id = -1;
    vector<vector<int>> group;
    Place(group);
    for (int gp_id = 0; gp_id < group.size(); gp_id++)
    {
        for (auto bid : group[gp_id])
        {
            if (bid == blk_id)
            {
                reside_gp_id = gp_id;
                break;
            }
        }

        if (reside_gp_id != -1)
        {
            break;
        }
    }

    return reside_gp_id;
}

void AzureLRCTradeoff::generateMatrix(int *matrix, int k, int l, int r, int w)
{
    int n = k + l + r;
    memset(matrix, 0, n * k * sizeof(int));

    // data blocks: set first k lines
    for (int i = 0; i < k; i++)
    {
        matrix[i * k + i] = 1;
    }

    // local parity: set the following l lines as local parity
    int nr = k / l;
    for (int i = 0; i < l; i++)
    {
        for (int j = 0; j < nr; j++)
        {
            matrix[(k + i) * k + i * nr + j] = 1;
        }
    }

    // Cauchy matrix
    int *p = &matrix[(_k + l) * _k];
    for (int i = k + l; i < n; i++)
    {
        for (int j = 0; j < k; j++)
        {
            *p++ = galois_single_divide(1, i ^ j, 8);
        }
    }
}

void AzureLRCTradeoff::Place(vector<vector<int>> &group)
{
    int _b = _k / _l;

    // Step 1: collcoate blocks from the same local group
    for (int lg_id = 0; lg_id < _l; lg_id++)
    {
        // collocate data blocks / local parity blocks in the rack
        for (int cl_id = 0; cl_id < _eta; cl_id++)
        {
            vector<int> gp;

            // add at most g+1 blocks from lg_id to the rack
            for (int idx = 0; idx < _g + 1; idx++)
            {
                int db_id = lg_id * _b + cl_id * (_g + 1) + idx;
                if (db_id < (lg_id + 1) * _b)
                {
                    // add data block
                    gp.push_back(db_id);
                }
                else
                {
                    // add local parity block
                    int lpb_id = _k + lg_id;
                    gp.push_back(lpb_id);
                }
            }
            group.push_back(gp);
        }
    }

    // Step 2: collocate blocks from different local groups
    if (_eta * (_g + 1) < _b) // each local group still have some data blocks to collocate
    {
        int num_remaining_db = _b - _eta * (_g + 1);
        for (int idx = 0; idx < num_remaining_db; idx++)
        {
            vector<int> gp;
            for (int lg_id = 0; lg_id < _l; lg_id++)
            {
                int db_id = lg_id * _b + _eta * (_g + 1) + idx;
                gp.push_back(db_id);
            }
            group.push_back(gp);
        }
    }

    // Step 3: collocate local parity blocks
    if (!(_eta * (_g + 1) >= _b && _b % (_g + 1) != 0))
    {
        vector<int> gp;
        for (int lg_id = 0; lg_id < _l; lg_id++)
        {
            gp.push_back(_k + lg_id);
        }
        group.push_back(gp);
    }

    // Step 4: collocate global parity blocks
    vector<int> gp;
    for (int gpb_id = 0; gpb_id < _g; gpb_id++)
    {
        gp.push_back(_k + _l + gpb_id);
    }
    group.push_back(gp);
}
