## Arty's programm to search Zeta Coefficient triples with minimal collisions

#### compile
Compile the program as follows:

```console
clang-13 minimize_collisions.cpp -o ./minimize_collisions -O3 -m64 -std=c++20 -lstdc++ -lm -ldl -pthread
```

#### run
Run the program as follows:

```console
./minimize_collisions real_quad_fields_1_2.csv a_ > collisions.txt
```
