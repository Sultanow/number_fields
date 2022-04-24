Using the file 91.6 M large file `final_dataset_1vs3_(2000)_coeffs.csv` which contains not only prime coefficients, we minimized according three cost functions:

* `C_1 = common_count / (#pure_Green + #pure_Red)`
* `C_2 = common_count - (#pure_Green + #pure_Red)`
* `C_3 = 2/(1/#pure_Green + 1/#pure_Red)`

which is in our C++ Syntax:

* `C_1 = cmn_cnt / (c0_cnt - cmn_cnt + c1_cnt - cmn_cnt)`
* `C_2 = cmn_cnt - (c0_cnt - cmn_cnt + c1_cnt - cmn_cnt)`
* `C_3 = 2/(1/(c0_cnt - cmn_cnt) + 1/(c1_cnt - cmn_cnt))`

since `#pure_Green = c0_cnt - cmn_cnt` and `#pure_Red = c1_cnt - cmn_cnt`.
