cd build
cmake -DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg ..
make -j
./attui ~/Desktop/att/nbody/ui/
echo "End program. Opening file."
gprof attui gmon.out | gprof2dot | ../scripts/clean_prof.py | dot -Tpng -o ../scripts/output.png
rm gmon.out
cd ..
xdg-open scripts/output.png
