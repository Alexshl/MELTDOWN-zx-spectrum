#!/usr/bin/env bash
set -euo pipefail

CMD="${1:-build}"

case "$CMD" in
    build)
        # Run the inner Makefile that only knows about zcc, no host tooling
        make -f Makefile.inner
        ;;
    shell)
        shift || true
        exec bash "$@"
        ;;
    smoke)
        shift || true
        exec /src/tools/smoke.sh "$@"
        ;;
    integration)
        shift || true
        # If a specific scenario is given, run it; otherwise loop all scenarios
        if [[ $# -gt 0 ]]; then
            exec /src/tools/integration/runner.sh "$@"
        else
            PASS=0
            FAIL=0
            for SCN in /src/tools/integration/scenarios/*.json; do
                echo "--- running: $SCN"
                if /src/tools/integration/runner.sh "$SCN"; then
                    PASS=$((PASS + 1))
                else
                    FAIL=$((FAIL + 1))
                fi
            done
            echo "integration: $PASS passed, $FAIL failed"
            [[ $FAIL -eq 0 ]]
        fi
        ;;
    trace)
        shift || true
        exec /src/tools/trace.sh "$@"
        ;;
    disasm)
        shift || true
        exec /src/tools/disasm.sh "$@"
        ;;
    disasm-alt)
        shift || true
        exec /src/tools/disasm-alt.sh "$@"
        ;;
    investigate)
        shift || true
        exec /src/tools/investigate.sh "$@"
        ;;
    *)
        echo "Usage: $0 {build|shell|smoke|integration|trace|disasm|disasm-alt|investigate}" >&2
        exit 1
        ;;
esac
