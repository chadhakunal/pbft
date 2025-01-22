#!/bin/bash
FILE_ID="1Cj06sjBfH7BVFPcMA65v7gBmzR42RQkP"
OUTPUT_FILE="test_cases.csv"
gdown "https://drive.google.com/uc?export=download&id=$FILE_ID" -O $OUTPUT_FILE
mv test_cases.csv client/tests/test-1.csv
