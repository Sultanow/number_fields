## Arty's programm to search Zeta Coefficient triples with minimal collisions

#### compile
Compile the program as follows:

```console
clang-13 minimize_collisions.cpp -o ./minimize_collisions -O3 -m64 -std=c++20 -lstdc++ -lm -ldl -pthread
```

#### run
Delete at first any precomputed file.
Then, run the program as follows:

```console
./minimize_collisions "final_dataset_1vs3_(2000)_coeffs.csv" a_
```

#### output
Three files are generated: green_red_columns_names.txt (1), green_red_precomputed.dat (2) and green_red_answer.txt (3).
The last file contains the final results.
