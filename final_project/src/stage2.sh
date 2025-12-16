g++ refiner_main.cpp -o refiner -O3 -std=c++11
# g++ -O2 -std=c++17 refiner.cpp -DREFINE_STANDALONE -o refiner

./refiner ../input/case01-input.txt ../output/ver_08/case01.out ./result.out
python3 ../visualize.py ../input/case01-input.txt ./result.out -o case01.png