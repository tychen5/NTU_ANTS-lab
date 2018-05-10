# 從VirusTotal取得Report 

說明：

* 透過VirusTotal API去查詢指定malware的report
* 若查不到report則會產生一個[noReport_list.txt]，裡面會有查不到report的Hash，再把這些hash對應的binary用scan API上傳到VT

# 程式使用 - 查詢Report

```
執行 [VirusTotalQueryExample.ipynb] 這支程式，就可以取得指定目錄底下所有hooklog的report。
* 檔名為hash_pid.trace.hooklog時可直接執行
* 檔名若為其他，則需修改程式[VirusTotalQuery.ipynb] 裡的 [def listMalwrHash()]

VirusTotalQueryExample變數解釋
* dataRootPath : 要查詢的資料集路徑
* outputDirPath : 要輸出的檔案路徑 （結尾須為 '/' ）
* requestLimit : VirusTotal給的key權限每分鐘的quota
* noReportList_fileName : noReport的hash值清單檔名，會在outputDirPath中產生對應檔名

注意：
* 在VirusTotalQuery.ipynb中，有privateKey的參數可以設定，若要更換key可修改'apikey'的值
* 'allinfo'的值為'1'是指要拿全部的資料回來（包含firstSeen等資訊）

```

# 程式使用 - 上傳新的binary （程式碼待補充）

```
透過官方文件可看到有個api叫"file/scan"
可以模仿API的使用，將[noReport_list.txt]中的hash值對應的binary上傳
```