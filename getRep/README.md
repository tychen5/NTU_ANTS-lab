# Sequence-Analysis
A analysis program about analyzing malware trace hooklog.
Propose a sequence-based clustering algorithm to analyze malwares.

(revised from https://github.com/weichih-c/Sequence_Analysis)

**Tested Environmnet**
* Python3.6 (anaconda)
* Ubuntu 16.04.5

## 2018/09/22 ##
### 分工 ###
* family 1.~15. => 智誠
* family 16.~40. => 子庭
* family 41.~80. => 鈞岱

### 輸出 ###
* 一個家族底下會有很多個資料夾(forest)
  * 每個資料夾代表一個tree
   * 裡面會有該tree對應的traces(hooklog)以及該tree的rep
   
### 作法 ###
1. 先跑RasMMAExample.ipynb並產生出pickle
2. 再跑CollectForestInfo.ipynb
    * 利用
