# usage:
# num_files h5_filenames updates

import numpy as np
import h5py
import sys
import os
from tqdm import tqdm
import pandas as pd
from keyname import keyname as kn
from fileshash import fileshash as fsh
import re
from collections import Counter
from joblib import delayed, Parallel
import json

num_files = int(sys.argv[1])
filenames = sys.argv[2:num_files+2]
updates = [int(v) for v in sys.argv[num_files+2:]]

# check all data is from same software source
assert len({kn.unpack(filename)['_source_hash'] for filename in filenames}) == 1

def BorderAge(filename):

    file = h5py.File(filename, 'r')
    indices = {
        idx : i
        for i, idx in enumerate(np.array(file['Index']['own']).flatten())
    }
    neighs = [
        np.array(file['Index']['dir_'+str(dir)]).flatten()
        for dir in range(4)
    ]
    nlev = int(file.attrs.get('NLEV'))

    res = []
    for update in tqdm(updates):
        chans = np.array(
            file['Channel']['lev_'+str(nlev-1)]['upd_'+str(update)]
        ).flatten()
        next_chans = np.array(
            file['Channel']['lev_'+str(nlev-1)]['upd_'+str(update+8)]
        ).flatten()

        lives = np.array(file['Live']['upd_'+str(update)]).flatten()
        next_lives = np.array(file['Live']['upd_'+str(update+8)]).flatten()
        cages = np.array(file['CellAge']['upd_'+str(update)]).flatten()
        next_cages = np.array(file['CellAge']['upd_'+str(update+8)]).flatten()
        pvchs = np.array(file['PrevChan']['upd_'+str(update)]).flatten()
        ppos = np.array(file['ParentPos']['upd_'+str(update)]).flatten()
        next_ppos = np.array(file['ParentPos']['upd_'+str(update+8)]).flatten()
        cage = np.array(file['CellAge']['upd_'+str(update)]).flatten()
        stock = np.array(file['Stockpile']['upd_'+str(update + 1)]).flatten()
        traffics = [
            np.array(file['InboxTraffic']['dir_'+str(dir)]['upd_'+str(update + 1)]).flatten()
            for dir in range(4)
        ]


        for i in range(len(indices)):
            for neigh, traffic in zip(neighs, traffics):
                if (
                    lives[i]
                    and lives[indices[neigh[i]]]
                    and next_lives[i]
                    and next_lives[indices[neigh[i]]]
                    and chans[indices[neigh[i]]] != chans[i]
                    and (
                        # cell replaced
                        next_cages[i] < 8
                        or next_cages[indices[neigh[i]]] < 8
                    )
                    # no propagule parent/propagule child
                    # relationship registered
                    and pvchs[i] != chans[indices[neigh[i]]]
                    and pvchs[indices[neigh[i]]] != chans[i]
                    and not ( # not cell parent
                        ppos[i] == indices[neigh[i]]
                        and cage[i] < cage[indices[neigh[i]]]
                    ) and not ( # not cell child
                        ppos[indices[neigh[i]]] == indices[i]
                        and cage[indices[neigh[i]]] < cage[i]
                    )):
                        res.append({
                            'Cell Age'
                                : cages[i] + 8,
                            'Inplace' : (
                                # A | B -> A | B
                                chans[i] == next_chans[i]
                                and chans[indices[neigh[i]]] == next_chans[indices[neigh[i]]]
                            ),
                            'Channel Swap' : (
                                # A | B -> A | A or B | B
                                chans[indices[neigh[i]]] == next_chans[i]
                                or chans[i] == next_chans[indices[neigh[i]]]
                            ),
                            'Channel Change' : (
                                # A | B -> A | C or C | B
                                (
                                    chans[indices[neigh[i]]] != next_chans[indices[neigh[i]]]
                                    and chans[i] != next_chans[indices[neigh[i]]]
                                ) or (
                                    chans[indices[neigh[i]]] != next_chans[i]
                                    and chans[i] != next_chans[i]
                                )
                            ),
                            'External Channel Change' : (
                                # safety: no ppos = -1
                                next_ppos[indices[neigh[i]]] != 2**32 - 1
                                and next_ppos[i] != 2**32 - 1
                                # channel change
                                # AND some other channel group was the
                                # parent
                                # e.g., C's parent wasn't A or B
                                and ((
                                    chans[indices[neigh[i]]] != next_chans[indices[neigh[i]]]
                                    and chans[i] != next_chans[indices[neigh[i]]]
                                    # other channel group was parent
                                    and chans[next_ppos[indices[neigh[i]]]] != chans[indices[neigh[i]]]
                                    and chans[next_ppos[indices[neigh[i]]]] != chans[i]
                                ) or (
                                    chans[indices[neigh[i]]] != next_chans[i]
                                    and chans[i] != next_chans[i]
                                    # other channel group was parent
                                    and chans[next_ppos[i]] != chans[indices[neigh[i]]]
                                    and chans[next_ppos[i]] != chans[i]
                                ))
                            ),
                            'Border Age'
                                : 8 + min(cages[i], cages[indices[neigh[i]]]),
                            'Update' : update,
                        })

    return res

def SafeBorderAge(filename):
    try:
        return BorderAge(filename)
    except Exception as e:
        print("warning: corrupt or incomplete data file... skipping")
        print("   ", e)
        return None


print("num files:" , len(filenames))

outfile = kn.pack({
    'title' : 'borderfall',
    '_data_hathash_hash' : fsh.FilesHash().hash_files(filenames),
    '_script_fullcat_hash' : fsh.FilesHash(
                                file_parcel="full_parcel",
                                files_join="cat_join"
                            ).hash_files([sys.argv[0]]),
    '_source_hash' :kn.unpack(filenames[0])['_source_hash'],
    'ext' : '.csv'
})

pd.DataFrame.from_dict([
    {
        **{
            'Genotype' : (
                'Wild Type' if 'id=1' in filename
                else 'Messaging Knockout' if 'id=2' in filename
                else None
            ),
            'Seed' : kn.unpack(filename)['seed'],
        },
        **entry,
    }
    for filename, res in zip(
        filenames,
        Parallel(n_jobs=-1)(
            delayed(SafeBorderAge)(filename)
            for filename in tqdm(filenames)
        )
    )
    for entry in res
]).to_csv(outfile, index=False)

print('Output saved to', outfile)
