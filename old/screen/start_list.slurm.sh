#!/bin/bash
########## Define Resources Needed with SBATCH Lines ##########
#SBATCH --time=4:00:00
#SBATCH --array=0-63
#SBATCH --mem=16G
#SBATCH --ntasks 1
#SBATCH --cpus-per-task 8
#SBATCH --job-name s-start
#SBATCH --account=devolab
#SBATCH --output="/mnt/home/mmore500/slurmlogs/slurm-%A_%a.out"
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=mmore500@msu.edu

################################################################################
echo
echo "Setup Exit and Error Traps"
echo "--------------------------"
################################################################################

function on_exit() {

  echo
  echo "Run Exit Trap"
  echo "-------------"

  qstat -f ${SLURM_JOB_ID}

  # prepare python environment
  module purge; module load GCC/7.3.0-2.30 OpenMPI/3.1.1 Python/3.6.6
  source "/mnt/home/mmore500/myPy/bin/activate"

  echo "   SECONDS" $SECONDS
  echo "   MINUTES" $(python3 -c "print( ${SECONDS}/60 )")
  echo "   HOURS  " $(python3 -c "print( ${SECONDS}/3600 )")

  cp ${SLURM_LOGPATH} .

}

function on_error() {

  echo
  echo "Run Error Trap (FAIL)"
  echo "---------------------"

  echo "   EXIT STATUS ${1}"
  echo "   LINE NO ${2}"
  echo "---------------------"
  echo

  cp ${SLURM_LOGPATH} "/mnt/home/mmore500/err_slurmlogs"

  qstat -f ${SLURM_JOB_ID}                                                     \
    >> "/mnt/home/mmore500/err_slurmlogs/${SLURM_LOGFILE}"

  echo "---------------------"
  echo

}

trap 'on_error $? $LINENO' ERR
trap "on_exit" EXIT

################################################################################
echo
echo "Prepare Env Vars"
echo "----------------"
################################################################################

SEED_OFFSET=2000
SEED=$((SLURM_ARRAY_TASK_ID + SEED_OFFSET))
CUR_STEP=1000

OUTPUT_DIR="/mnt/scratch/mmore500/dishtiny-screen2/seed=${SEED}+step=${CUR_STEP}"
CONFIG_DIR="/mnt/home/mmore500/dishtiny/screen/"

echo "   SEED" $SEED
echo "   CUR_STEP" $CUR_STEP
echo "   OUTPUT_DIR" $OUTPUT_DIR
echo "   CONFIG_DIR" $CONFIG_DIR

export SLURM_LOGFILE="slurm-${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}.out"
export SLURM_LOGPATH="/mnt/home/mmore500/slurmlogs/${SLURM_LOGFILE}"

echo "   SLURM_LOGFILE" $SLURM_LOGFILE
echo "   SLURM_LOGPATH" $SLURM_LOGPATH

################################################################################
echo
echo "Setup Work Dir"
echo "--------------"
################################################################################

rm -rf ${OUTPUT_DIR}/* || echo "   not a redo"
mkdir -p ${OUTPUT_DIR}
cp -r ${CONFIG_DIR}/* ${OUTPUT_DIR}
cd ${OUTPUT_DIR}
echo "   PWD" $PWD

################################################################################
echo
echo "Do Work"
echo "-------"
################################################################################

module purge; module load GCC/8.2.0-2.31.1 OpenMPI/3.1.3 HDF5/1.10.4;

export OMP_NUM_THREADS=2
export SECONDS

for POPULATION in {1..4}; do

  ./dishtiny                                                                   \
    -SEED $((${SEED} + ${POPULATION} * ${SEED_OFFSET}))                        \
    >"title=run+population=${POPULATION}+ext=.log" 2>&1 &

done

wait

################################################################################
echo
echo "Submit Next Job"
echo "---------------"
################################################################################

# export information so jinja can use it
export seed=$SEED
export last_step=$CUR_STEP # aka cur step wrt this script
export cur_step=$(( $CUR_STEP + 1 )) # aka next step wrt this script

echo "   seed" $seed
echo "   last_step" $last_step
echo "   cur_step" $cur_step

# prepare python environment
module purge; module load GCC/7.3.0-2.30 OpenMPI/3.1.1 Python/3.6.6
source "/mnt/home/mmore500/myPy/bin/activate"

# run jinja on template
j2 -o run_list.slurm.sh run_list.slurm.sh.jinja

if  [ $CUR_STEP -lt 2000 ]; then
  if [ $(ls *.json.cereal | wc -l) -eq 4 ]; then
    sbatch run_list.slurm.sh                                                   \
    && echo "   job submit success!"                                           \
    || (echo "   job submit failure (FAIL)" && exit 1)
  else
    echo "   missing final population... job incomplete? (FAIL)"
    exit 2
  fi
else
  echo "   nevermind, all done! (COMPLETE)"
fi

################################################################################
echo
echo "Done! (SUCCESS)"
echo "---------------"
################################################################################
