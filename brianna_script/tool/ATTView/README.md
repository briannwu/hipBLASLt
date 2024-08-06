## ATT VIEWER ##
# For Internal use only. Do not distribute. Tool in development.

# Dependencies: #
 * Qt5
 * ROCm 5.5

Optional:
 * gprof
 * gprof2dot
 * dot (graphviz)

# Build and run #
./sbuild.sh
./build/attui

# (Optional) Running for a default ATT folder #
./build/attui <path_to_att_processed_data> <path_to_root_project_folder>

# (Optimization, for developers) Run gprof trace analysis #
./trace.sh

# (Tests) Running the tests #
./run_tests.sh
