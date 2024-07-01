#include "AzureLRCFlat.hh"

AzureLRCFlat::AzureLRCFlat(int n, int k, int w, int opt, vector<string> param)
{
    _n = n;
    _k = k;
    _w = w;
    _opt = opt;
    if (param.size() != 2)
    {
        printf("AzureLRCFlat::error invalid params (l,g)\n");
    }

    // two parameters in <param>
    // 1. l (number of local parity blocks)
    // 2. g (number of local parity blocks)
    _l = atoi(param[0].c_str());
    _g = atoi(param[1].c_str());

    _encode_matrix = (int *)malloc(_n * _k * sizeof(int));
    generateMatrix(_encode_matrix, _k, _l, _g, 8);

    printf("AzureLRCFlat::_encode_matrix:\n");
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

ECDAG *AzureLRCFlat::Encode()
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

ECDAG *AzureLRCFlat::Decode(vector<int> from, vector<int> to)
{
    ECDAG *ecdag = new ECDAG();

    if (to.size() == 1)
    {
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
    }
    else
    {
        printf("AzureLRCFlat:: multiple failures not implemented\n");
    }
    return ecdag;
}

void AzureLRCFlat::generateMatrix(int *matrix, int k, int l, int r, int w)
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

void AzureLRCFlat::Place(vector<vector<int>> &group)
{
    group.clear();
    for (int i = 0; i < _n; i++)
    {
        vector<int> gp;
        gp.push_back(i);
        group.push_back(gp);
    }
}
