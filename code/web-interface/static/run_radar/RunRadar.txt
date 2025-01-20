@echo off
set "DCA_CLI=DCA1000EVM_CLI_Control.exe"
set "MMWAVE_EXAMPLE=mmwavelink_example.exe"
set "CONFIG=cf.json"

REM Step 1: Run FPGA setup
echo Starting FPGA configuration...
"%DCA_CLI%" fpga "%CONFIG%"
if errorlevel 1 (
    echo Error: FPGA configuration failed.
    exit /b 1
)

REM Step 2: Run record configuration
echo Starting record configuration...
"%DCA_CLI%" record "%CONFIG%"
if errorlevel 1 (
    echo Error: Record configuration failed.
    exit /b 1
)

REM Step 3: Run mmwave_example.exe and start recording in parallel
echo Starting mmWave example and DCA recording...
start "mmWave Example" "%MMWAVE_EXAMPLE%"
start "DCA Start Record" "%DCA_CLI%" start_record "%CONFIG%"

REM Final message
echo All commands have been started. Press any key to exit the script.
pause > nul