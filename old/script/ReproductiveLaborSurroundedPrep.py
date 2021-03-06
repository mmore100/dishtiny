# usage:
# first_update last_update h5_filenames

import numpy as np
import h5py
import sys
import scipy.stats as stats
from tqdm import tqdm
import os
import pandas as pd
from keyname import keyname as kn
from fileshash import fileshash as fsh
from joblib import delayed, Parallel

first_update = int(sys.argv[1])
last_update = int(sys.argv[2])
filenames = sys.argv[3:]

# check all data is from same software source
assert len({kn.unpack(filename)['_source_hash'] for filename in filenames}) == 1

def CalcSurroundedRate(filename):
    file = h5py.File(filename, 'r')
    nlev = int(file.attrs.get('NLEV'))
    return np.mean([
        sum(ro[idx] != -1 for ro in ros)
        for ch, pc, dirs, live, ros in [
                (
                np.array(
                    file['Channel']['lev_'+str(nlev-1)]['upd_'+str(upd)]
                ).flatten(),
                np.array(
                    file['PrevChan']['upd_'+str(upd)]
                ).flatten(),
                [np.array(
                    file['Index'][dir_key]
                ).flatten() for dir_key in file['RepOutgoing']],
                np.array(
                    file['Live']['upd_'+str(upd)]
                ).flatten(),
                [np.array(
                    file['RepOutgoing'][dir_key]['upd_'+str(upd)]
                ).flatten() for dir_key in file['RepOutgoing']]
                )
            for upd in range(first_update, last_update)
        ]
        for idx in range(file['Index']['own'].size)
        if live[idx] and all(ch[idx] == ch[dir[idx]] for dir in dirs)
    ])

def SafeCalcSurroundedRate(filename):
    try:
        return CalcSurroundedRate(filename)
    except Exception as e:
        print("warning: corrupt or incomplete data file... skipping")
        print("   ", e)
        return None

def CalcNotSurroundedRate(filename):
    file = h5py.File(filename, 'r')
    nlev = int(file.attrs.get('NLEV'))
    return np.mean([
        sum(ro[idx] != -1 for ro in ros)
        for ch, pc, dirs, live, ros in [
                (
                np.array(
                    file['Channel']['lev_'+str(nlev-1)]['upd_'+str(upd)]
                ).flatten(),
                np.array(
                    file['PrevChan']['upd_'+str(upd)]
                ).flatten(),
                [np.array(
                    file['Index'][dir_key]
                ).flatten() for dir_key in file['RepOutgoing']],
                np.array(
                    file['Live']['upd_'+str(upd)]
                ).flatten(),
                [np.array(
                    file['RepOutgoing'][dir_key]['upd_'+str(upd)]
                ).flatten() for dir_key in file['RepOutgoing']]
                )
            for upd in range(first_update, last_update)
        ]
        for idx in range(file['Index']['own'].size)
        if live[idx] and any(ch[idx] != ch[dir[idx]] for dir in dirs)
    ])

def SafeCalcNotSurroundedRate(filename):
    try:
        return CalcNotSurroundedRate(filename)
    except Exception as e:
        print("warning: corrupt or incomplete data file... skipping")
        print("   ", e)
        return None

print("num files:" , len(filenames))

outfile = kn.pack({
    '_data_hathash_hash' : fsh.FilesHash().hash_files(filenames),
    '_script_fullcat_hash' : fsh.FilesHash(
                                    file_parcel="full_parcel",
                                    files_join="cat_join"
                                ).hash_files([sys.argv[0]]),
    '_source_hash' :kn.unpack(filenames[0])['_source_hash'],
    'title' : 'reproductive_labor_surrounded',
    'ext' : '.csv'
})

pd.DataFrame.from_dict([
    {
        'Treatment' : kn.unpack(filename)['treat'],
        'Reproduction Rate' : res,
        'Channel 1 Surrounded' : 'True',
        'First Update' : first_update,
        'Last Update' : last_update
    }
    for res, filename in zip(
        Parallel(n_jobs=-1)(
            delayed(SafeCalcSurroundedRate)(filename)
            for filename in tqdm(filenames)
        ), filenames
    )
] + [
    {
        'Treatment' : kn.unpack(filename)['treat'],
        'Reproduction Rate' : res,
        'Channel 1 Surrounded' : 'False',
        'First Update' : first_update,
        'Last Update' : last_update
    }
    for res, filename in zip(
        Parallel(n_jobs=-1)(
            delayed(SafeCalcNotSurroundedRate)(filename)
            for filename in tqdm(filenames)
        ), filenames
    )
]).to_csv(outfile, index=False)

print('Output saved to', outfile)
