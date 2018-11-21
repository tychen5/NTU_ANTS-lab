# Sequence-Analysis
A analysis program about analyzing malware trace hooklog.
Propose a sequence-based clustering algorithm to analyze malwares.

- dataset: https://drive.google.com/drive/u/0/folders/1PoAn_RuJOyTvJoFkhGbxGo8necwQvHRd
  - 180920_aries_family_simplified_15up.zip
- revised from: https://github.com/weichih-c/Sequence_Analysis
- documentation: https://mega.nz/#!N1NAXSjZ!lHyUl2gZFTjbRpa1PSTSGQ3z3uvQelgWKaGXPq-hxOQ
- RasMMA illustration: https://mega.nz/#!0hFkUI7R!JZktrdDB-LrYtcYBwygJgvG5OT_VAwXNtdMQ5b_tUvg

**Tested Environmnet**
* Python3.6 (latest anaconda)
* Ubuntu 16.04.5


## 2018/11/17 UPDATE ##
**目標:** 

1. 需將REP中的各motif拆開，並加入\<BOS\>於第一個motif的開頭，且於最後一個motif結尾加入\<EOS\>，而motif跟motif之間要加入\<MOS\>
2. 利用utils/api_enc2.pkl檔案( https://github.com/tychen5/NTU_ANTS-lab/blob/master/getRep/utils/api_enc2.pkl )將\<BOS\>、motifs、\<MOS\>、\<EOS\>都轉換依據進行one-hot encoding變成2D numpy array
3. 另存一個具有api call與parameters的string list，在不同的motif list之間加入'\<MOS\>'，把REP的各list合成一個1D list
  
=>注意1. 2.僅使用api function name；3.則為整個invocation call(api name+所有parameters)不用轉換為onehot

**做法:**

1. 利用CollectForestInfo.ipynb讀入Tree的REP之intermediate.pickle、residual.pickle初始化建構子(CollectForestInfo)，再利用裡面的function: getRepMotifSequence()或是getRepAPISeq_dict()來獲得REP
2. 將REP list中的api function name取出，並於第一個motif開頭加入\<BOS\>，motif間加入\<MOS\>，最後一個motif加入\<EOS\>，且將所有REP的list串在一起變成1D list
3. 利用api_enc2.pkl將\<BOS\>、\<MOS\>、\<EOS\>及api names轉換為one-hot(例如: df['\<MOS\>'].values)，再將整個list轉換為2D numpy array存成api_name.pickle (np.shape=(REP-length+\<BOS\>、\<MOS\>s、\<EOS\>,32))
4. 一樣利用getRepMotifSequence()或是或是getRepAPISeq_dict()取得REP(function name & parameters)以後，將motif與motif之間於list多加入\"\<MOS\>\"字串型態的元素，最後也是將該tree的REP轉換為1D的list，每個element就是一個完整的api invocation call(包含parameters)或是\<BOS\>、\<MOS\>、\<EOS\>，都是字串型態，再將該1D string list存為parameter_rep.pickle (別轉成numpy array)
5. 將兩個pickle儲存於對應的tree目錄底下(tree-rep-logs-profile/family/tree/***.pickle 例如:tree-rep-logs-profile/allaple_0.8/G1299/api_name.pickle)
6. 將tree-rep-logs-profile資料夾壓縮成zip，再將zip檔案名稱重新命名加上family範圍，上傳至https://drive.google.com/drive/u/0/folders/1T2MdJ7nAwLZKBuISGw5-SmzkYADtJ49s
  
=>注意: special tokens包含\<BOS\>、\<MOS\>、\<EOS\>請務必加入並一起轉換為one-hot；parameter_rep.pickle中的\<BOS\>、\<MOS\>、\<EOS\>則不用轉換，保留字串型態於list中即可

=>Deadline: 11/22 18:00

## 2018/09/30 ##
當下面09/22兩個步驟做法完成以後

接下來要將各自所負責的家族，各tree的rep pickle讀出來，會得到1D的list，例如: `['RegQueryValue#PR@HKLM@sys_curCtlSet_ctl_sessionManager\\*#PR@SUBK@criticalsectiontimeout#PR@0#PR@12f9b0#Ret#0',
  'RegQueryValue#PR@HKLM@soft_ms_ole\\*#PR@SUBK@rwlockresourcetimeout#PR@0#PR@12f9b4#Ret#P',
  'LoadLibrary#PR@SYS@wininet@DLL#Ret#P',
  'LoadLibrary#PR@SYS@advapi32@DLL#Ret#P',
  'LoadLibrary#PR@SYS@advapi32@DLL#Ret#P','CopyFile#PR@ARB@DLL#PR@ARB@DLL#Ret#N',...]`

則要將前面的api call萃取出來變成: `[RegQueryValue,RegQueryValue,LoadLibrary,LoadLibrary,LoadLibrary,CopyFile,...]`

接下來要對之進行one-hot encoding的轉換並加上start token、comma token、endding token: `<BOS> RegQueryValue RegQueryValue LoadLibrary LoadLibrary LoadLibrary CopyFile ... <EOS>`
   - one-hot encoding: 利用output/api_enc.pkl 檔案作為轉換依據，load pickle後為一dataframe，利用api作為key值來轉換，轉換方式為df['XXX'].values可得該XXX的numpy array。如: df['\<MOS\>'].values可得array([0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
   
### 做法 ###
* 將前兩步驟該tree的rep之pickle讀入以後可獲得api call sequences的list
* 同時讀入api_enc.pkl檔案可獲得one-hot dataframe: https://github.com/tychen5/NTU_ANTS-lab/tree/master/malwareTagging/output
* 將api call sequences處理後可得去除parameters僅有api的list
* 接下來將list每個元素利用上述one-hot encoding方式轉換為2D numpy array
* 接下來看該tree有幾個hooklogs進行duplicate該2D array，因此會變成3D numpy array (即便只有一個trace也要expand dimension變成3D)
* 最終要輸出numpy的shape為(該tree有幾個members, 該tree的rep長度,37)

### 輸出 ###
* 為每個tree產生一個3D的numpy array pickle
* 輸出資料階層:family=>tree=> **hooklogs & REP's 3D numpy array pickle** (一起放在同一個資料夾裡面) 檔案名稱:rep-numpy-3D.pickle
* 上傳到Google drive: https://drive.google.com/drive/u/0/folders/1x-zt2ZZnpMwfDKH22ORa3Tvpm6P0UxFS
* 壓縮請用zip最外層資料夾請統一名字tree-rep-logs/，最後的完成3D numpy array請用.shape檢查是否為3D


## 2018/09/22 ##
### 分工 ###
* family 1.~15. => 智誠
* family 16.~40. => 子庭
* family 41.~80. => 鈞岱

### 輸出 ###
* 一個家族底下會有很多個資料夾(forest)
  * 每個資料夾代表一個tree
   * 裡面會有**該tree對應的traces(hooklog)** 以及該tree的rep (1D list的pickle檔案)
* 上傳到Google drive: https://drive.google.com/drive/u/0/folders/1x-zt2ZZnpMwfDKH22ORa3Tvpm6P0UxFS
   
### 作法 ###
1. 先跑RasMMAExample.ipynb並產生出pickle
2. 再跑CollectForestInfo.ipynb
    * 需讀入所產生出的intermediate.pickle跟residual.pickle以產生CollectForestInfo的建構子初始化
    * 利用**getTreeMembers**函式來獲得該family forest各tree的hooklogs => `for tree in forest:`
    * 利用**getRepAPISeq_dict()** 或是**getRepAPISeq**函式來獲取該family forest各tree的REP <= 1D list
    
### 參考程式碼 by 智誠
1. 把multi-process加進RasMMAExample.ipynb，可指定一個範圍的family number下去跑 (ex: 1~15), 並用shared memory queue紀錄錯誤訊息。
2. CollectForestInfo.ipynb裡面提供dump出各家族下各顆樹的hooklog以及rep sequence
3. 最終ouput的目錄結構如下：
```
- family_name
    - tree_name
        - *.trace.hooklog
        - rep.pickle

- allaple_0.8
    - G1286
        - 4a8581ee09a6f9794b3cafa0cbe493eb43604978e51dd460b2dfbbc3f344938b_3268.trace.hooklog
        - a70c1f66c37b0aa1f68a6bc7502b10a56a16a5e8ee01c41128a525891f166d1f_3220.trace.hooklog
        - rep.pickle
    - G1294
    ...
...
```

**大家最好資料夾和檔案命名可以一致，不然到時候學長要讀資料來training的時候會崩潰。**

**請至少資料夾階層數要一樣**
    
### HINT & Notice ###
1. 建議從數字大的家族開始跑以測試自己自動化程式的完整性與正確性
2. 太大的檔案可以透過一些統計的方法(iqr)把離群值(outlier)拿掉再來跑，不然會跑到天荒地老
3. 如果有出現error而且是因為某個hooklog檔案所導致的問題，可以先把該trace移除，放到該家族的exception資料夾，並且截圖註明原因(EX:編碼錯誤、檔案過大)
4. 可以改寫code變成for迴圈自己讓他自己跑完所有家族，或是改code利用multi processing來平行跑(數字越小的家族跑越久)



