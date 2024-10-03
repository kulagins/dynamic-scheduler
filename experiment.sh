#!/bin/bash
pwd
#!/bin/bash

echo "Arguments passed to the script: $*"


# Call the fonda_scheduler with no parameters
#./build/fonda_scheduler &
#server_pid=$!

# Check if the scheduler ran successfully
#if [ $? -ne 0 ]; then
#    echo "fonda_scheduler failed"
#    exit 1
#fi

# Sleep for 2 seconds before running the Python script
sleep 2


cd ../runtime-system || exit

# Run the python script with all the arguments passed to this bash script
echo "Executing: python -m gear $*"
pwd
python -m gear "$@"

#Check if the python script ran successfully
if [ $? -ne 0 ]; then
    echo "Python script failed"
    exit 1
fi

#kill $server_pid


echo "Both executables ran successfully."