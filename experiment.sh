#!/bin/bash
pwd
#!/bin/bash

echo "Arguments passed to the script: $*"

cd ../runtime-system || exit

# Run the python script with all the arguments passed to this bash script
python "$@"

# Check if the python script ran successfully
if [ $? -ne 0 ]; then
    echo "Python script failed"
    exit 1
fi


# Call the fonda_scheduler with no parameters
./build/fonda_scheduler

# Check if the scheduler ran successfully
if [ $? -ne 0 ]; then
    echo "fonda_scheduler failed"
    exit 1
fi

# Sleep for 2 seconds before running the Python script
sleep 2

echo "Both executables ran successfully."