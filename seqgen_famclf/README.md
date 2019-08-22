# 基於自注意力機制產生重要執行序的惡意程式家族分類系統 (Malware Family Classification System based on Attention-based Characteristic Execution Sequence)
**「自動化惡意程式重要執行序辨識系統」**


## Trained Model
* https://drive.google.com/drive/folders/1dzucL1MuvYtRaV2wevjVgD3_CIfJRDOD
    * \*.h5 : only parameters
    * \*_all.h5 : with architecture & model parameters
* system final model: byterep_0706_gruatt_sent2vec
* Embedder Experiment models: o2o_embEXP_gru_selfatt/
* Encoder Experiment models: o2o_encEXP_gru_selfatt/
* embedding layer initialize weight: emb_init_matrix.pkl
   * pretrained BERT model & corpus: BERT_pretrained_model/
* Sent2Vec trained model: Sent2Vec_pretrained_APIcalls_768model.bin

## Dataset
* train/valid/test/loner Set profiles: https://drive.google.com/drive/folders/197924KGc_hSUZeW8KZAhRIBADEODwVxz
* train/valid/test/loner Set representative exexcution pattern vector: https://drive.google.com/drive/folders/1eldxcQiYsHMiPscYZlmbSCss4qWuHZ0U

## 最原始完全未處理的profiles Dataset
* 一個sample對應一個家族的execution profiles準備done by子庭；惡意程式家族分類done by鈞岱。detail see: https://github.com/tychen5/NTU_ANTS-lab/tree/master/getRep
* ftp://XXX...45/zizi/廷易嚶嚶/   (帳密為自己登入superPC的帳密)
* loner Set: G0.zip
* train/dev/test Set: AnA_non-split_v4.zip 、 AnA_split_v4.zip
* profiling前處理統計資料: AnA_statistic.xlsx


## Source code documentation & interpretation
* 0001.Statistics_Prprocessing_DataSplit_LonerSetPrepare.ipynb
   * dataset的各項統計資訊 (process長度、rep長度、tree樣本數、tree執行程序數、一個sample有幾個process、一個family有幾個processes、一個family有幾個samples、一個family有幾個trees)
   * 資料前處理 (移除過長的profile)
   * 資料集切分 (train/dev/test)
   * 挑出loner set中有存在於dev的家族
* 0002.Sent2Vec corpus preparation & embedding evaluation.ipynb
   * 準備Sen2Vec corpus (sent2vec 的 code)
   * evaluate sent2vec vectors
   * 準備 embedding layer initial weight ( 處理BERT的code)
* 0003.Prepare Dataset Embedding Matrix & Feature Scaling.ipynb
   * 準備每個資料集(train/dev/test/loner Set)的embedding vectors: API function name encode、API invocation call embed
   * 對Sent2Vec vectors的每一個維度進行Z-score normalize
* 0004.NN model & Filter threshold selection.ipynb
   * 核心類神經網路模型訓練 (encoder的code)( hyper-parameter在哪裡設定)
   * 挑選所訓練模型最適合的filter threshold (by驗證資料集)(filter 的code)
   * Sec4.2 實驗1-1、1-2、1-3 (比較不同方法的Embedder、Encoder、Filter在測試資料集的表現F1、precision、recall、hammingLoss)
   * 輸出training process資訊 (train loss/ valid loss / train acc / valid acc)
* 0005.Representative execution pattern vector experiments & statistics.ipynb
   * 準備training set/validation set/testing set/loner set的r
   * Sec4.3 實驗2: testing set之BT與F歸類、各項統計圖表計算與輸出
   * Sec4.4 實驗3: loner set之F分類、各項統計圖表計算與輸出
   * Sec4.5 allaple可視化: 視覺化r資料準備
   * kNN分類測試資料集與loner資料集
   * BT centroid方式分類測試資料集與loner資料集

## Thesis Figures原始資料
**https://drive.google.com/drive/folders/1QRVf62rOCPoN9a8vkAk7YEPAB0oWj7mb**
* Fig3.13: ori_system_history_public.xlsx
* Fig4.11: threxp_F1_rec_pre_Hloss.xlsx
* Fig4.13: testing dataset EM accuracy.docx、EM_draw_allTest.xlsx、EM_draw_onlywREP.xlsx
* Fig4.14: FamilyMatch_rate_loner.xlsx中的Sheet1(2)
* Fig4.15: FamilyUNK_rate_loner.xlsx中的Sheet1(2)
* Fig4.16: Family_Train_haveREP_pid_ratio.xlsx
* Fig4.19: FamilyMatch_rate_loner.xlsx中的Sheet1(3)、fam_hash_woUNK_mis_num09.xlsx
* Fig4.20: FamilyMatch08_rate_loner.xlsx、fam_hash_woUNK_mis_num.xlsx
* Fig4.21: FamilyUNK_rate_loner.xlsx中的Sheet1(3)、fam_unkhash_num.xlsx
* Sec 4.5 figures: https://drive.google.com/drive/folders/1UOa_Dgi1yVbPtUtc4ppqaJG2J1BTfuE4


## Thesis Tables原始資料
**https://drive.google.com/drive/folders/1M7RIbL5j836jzg0D4w-rhTWl30yfhu7g**
* Table4.4: train_valid_test_fam_stat_correct.xlsx
* Table4.8: Oral_自動化惡意程式重要執行序行為辨識_TY.pptx中的P.85、Family_Train_allpid_ratio.xlsx
* Sec 4.3 |F|和|BT|的distribution: argmax_match_dist.xlsx中的Sheet1
* Sec 4.4 0.9 effective match |F|的distribution: topN09_predictFam_num_loner.xlsx
* Sec 4.4 0.8 effective match |F|的distribution: topN08_predictFam_num_loner.xlsx
* Sec 4.3 no representative execution pattern vector的sha256: test_no_rep_hash.xlsx
* Sec 4.4 no representative execution pattern vector的sha256: loner_no_rep_hash.xlsx
* Sec 4.3 mismatch Family的sha256: test_mismatch_fam_sha256.xlsx
* Sec 4.3 mismatch BT的sha256: test_mismatch_tree_sha256.xlsx
* Sec 4.4 0.9 mismatch Family的sha256: loner09_mismsatch_fam2.xlsx、fam_hash_woUNK_mis_num09.xlsx(含undecided)
* Sec 4.4 0.8 mismatch Family的sha256:loner08_mismsatch_fam.xlsx、fam_hash_woUNK_mis_num.xlsx(含undecided)
* sample很靠近的是來自哪些family 0.8: fam_loner_sampleNum08.xlsx、sample_belongFam08.xlsx
* sample很靠近的是來自哪些family 0.9: fam_loner_sampleNum09.xlsx、sample_belongFam09.xlsx
* loner 中family的分佈: loner_47fam_dist.xlsx

## Package List
PackageName             /               Version

- absl-py                            0.7.1
- alabaster                          0.7.12
- anaconda-client                    1.7.2
- anaconda-navigator                 1.9.7
- anaconda-project                   0.8.2
- apex                               0.1
- appdirs                            1.4.3
- asn1crypto                         0.24.0
- astor                              0.7.1
- astroid                            2.2.5
- astropy                            3.1.2
- atomicwrites                       1.3.0
- attrs                              19.1.0
- Automat                            0.7.0
- Babel                              2.6.0
- backcall                           0.1.0
- backports.os                       0.1.1
- backports.shutil-get-terminal-size 1.0.0
- bcrypt                             3.1.6
- beautifulsoup4                     4.7.1
- bitarray                           0.8.3
- bkcharts                           0.2
- blaze                              0.11.3
- bleach                             3.1.0
- bokeh                              1.0.4
- boto                               2.49.0
- boto3                              1.8.7
- botocore                           1.11.7
- Bottleneck                         1.2.1
- bz2file                            0.98
- certifi                            2019.3.9
- cffi                               1.12.2
- chardet                            3.0.4
- Click                              7.0
- cloudpickle                        0.8.1
- clyent                             1.2.2
- colorama                           0.4.1
- conda                              4.7.5
- conda-build                        3.17.8
- conda-package-handling             1.3.10
- conda-verify                       3.3.0
- constantly                         15.1.0
- contextlib2                        0.5.5
- Counter                            1.0.0
- cryptography                       2.6.1
- cycler                             0.10.0
- cymem                              2.0.2
- Cython                             0.29.6
- cytoolz                            0.9.0.1
- dask                               1.1.4
- datashape                          0.5.4
- de-core-news-sm                    2.0.0
- decorator                          4.4.0
- defusedxml                         0.5.0
- dill                               0.2.9
- distributed                        1.26.0
- docutils                           0.14
- dougu                              0.1
- en-core-web-sm                     2.0.0
- entrypoints                        0.3
- es-core-news-sm                    2.0.0
- et-xmlfile                         1.0.1
- fairseq                            0.6.2
- fastcache                          1.0.2
- fasttext                           0.8.22
- filelock                           3.0.12
- Flask                              1.0.2
- Flask-Cors                         3.0.7
- fr-core-news-sm                    2.0.0
- future                             0.17.1
- gast                               0.2.2
- gensim                             3.7.3
- gevent                             1.4.0
- glob2                              0.7
- gmpy2                              2.0.8
- google-pasta                       0.1.6
- greenlet                           0.4.15
- grpcio                             1.16.1
- h5py                               2.9.0
- heapdict                           1.0.0
- html5lib                           1.0.1
- hyperlink                          19.0.0
- idna                               2.7
- imageio                            2.5.0
- imagesize                          1.1.0
- imbalanced-learn                   0.4.3
- importlib-metadata                 0.0.0
- incremental                        17.5.0
- integrate                          1.2.1
- ipykernel                          5.1.1
- ipython                            7.4.0
- ipython-genutils                   0.2.0
- ipywidgets                         7.4.2
- isort                              4.3.16
- it-core-news-sm                    2.0.0
- itsdangerous                       1.1.0
- jdcal                              1.4
- jedi                               0.13.3
- jeepney                            0.4
- Jinja2                             2.10
- jmespath                           0.9.3
- joblib                             0.13.2
- jsonschema                         3.0.1
- jupyter                            1.0.0
- jupyter-client                     5.2.4
- jupyter-console                    6.0.0
- jupyter-core                       4.4.0
- jupyterlab                         0.35.4
- jupyterlab-launcher                0.13.1
- jupyterlab-server                  0.2.0
- jupyterthemes                      0.20.0
- Keras                              2.2.4
- Keras-Applications                 1.0.7
- keras-metrics                      1.1.0
- keras-multi-head                   0.20.0
- Keras-Preprocessing                1.0.9
- keras-self-attention               0.41.0
- keras-trans-mask                   0.1.0
- keras-transformer                  0.1.dev17+gb9d4e76
- keyring                            18.0.0
- kiwisolver                         1.0.1
- lazy-object-proxy                  1.3.1
- lesscpy                            0.13.0
- libarchive-c                       2.8
- lief                               0.9.0
- llvmlite                           0.28.0
- locket                             0.2.0
- logger                             1.4
- lxml                               4.2.6
- Markdown                           3.1.1
- MarkupSafe                         1.1.1
- matplotlib                         3.1.1
- mccabe                             0.6.1
- mistune                            0.8.4
- mkl-fft                            1.0.10
- mkl-random                         1.0.2
- mock                               2.0.0
- more-itertools                     6.0.0
- mosspy                             1.0.7
- mpmath                             1.1.0
- msgpack                            0.6.1
- msgpack-numpy                      0.4.3.2
- multipledispatch                   0.6.0
- murmurhash                         1.0.2
- navigator-updater                  0.2.1
- nb-conda                           2.2.1
- nb-conda-kernels                   2.2.2
- nbconvert                          5.4.1
- nbformat                           4.4.0
- networkx                           2.2
- nl-core-news-sm                    2.0.0
- nltk                               3.4
- nose                               1.3.7
- notebook                           5.7.8
- numba                              0.43.1
- numexpr                            2.6.9
- numpy                              1.16.4
- numpydoc                           0.8.0
- odo                                0.5.1
- olefile                            0.46
- openpyxl                           2.6.1
- packaging                          19.0
- pandas                             0.24.2
- pandocfilters                      1.4.2
- parso                              0.3.4
- partd                              0.3.10
- path.py                            11.5.0
- pathlib2                           2.3.3
- patsy                              0.5.1
- pause                              0.2
- pbr                                5.1.3
- pep8                               1.7.1
- pexpect                            4.6.0
- pickleshare                        0.7.5
- Pillow                             5.4.1
- pip                                19.1.1
- pkginfo                            1.5.0.1
- plac                               0.9.6
- pluggy                             0.9.0
- ply                                3.11
- preshed                            2.0.1
- progressbar2                       3.39.3
- prometheus-client                  0.6.0
- prompt-toolkit                     2.0.9
- protobuf                           3.7.1
- psutil                             5.6.1
- pt-core-news-sm                    2.0.0
- ptyprocess                         0.6.0
- py                                 1.8.0
- pyasn1                             0.4.5
- pyasn1-modules                     0.2.4
- pybind11                           2.2.4
- pycodestyle                        2.5.0
- pycosat                            0.6.3
- pycparser                          2.19
- pycrypto                           2.6.1
- pycurl                             7.43.0.2
- pyflakes                           2.1.1
- Pygments                           2.3.1
- PyHamcrest                         1.9.0
- pylint                             2.3.1
- pyodbc                             4.0.26
- pyOpenSSL                          19.0.0
- pyparsing                          2.3.1
- pyrsistent                         0.14.11
- PySocks                            1.6.8
- pytest                             4.3.1
- pytest-arraydiff                   0.3
- pytest-astropy                     0.5.0
- pytest-doctestplus                 0.3.0
- pytest-openfiles                   0.3.2
- pytest-remotedata                  0.3.1
- python-dateutil                    2.8.0
- python-utils                       2.3.0
- pytorch-ignite                     0.2.0
- pytorch-pretrained-bert            0.6.2
- pytz                               2018.9
- PyWavelets                         1.0.2
- PyYAML                             5.1
- pyzmq                              18.0.0
- QtAwesome                          0.5.7
- qtconsole                          4.4.3
- QtPy                               1.7.0
- recurrentshop                      1.0.0
- regex                              2018.1.10
- requests                           2.21.0
- rope                               0.12.0
- ruamel-yaml                        0.15.46
- s3transfer                         0.1.13
- sacrebleu                          1.3.1
- scikit-image                       0.14.2
- scikit-learn                       0.21.2
- scipy                              1.3.0
- seaborn                            0.9.0
- SecretStorage                      3.1.1
- Send2Trash                         1.5.0
- sent2vec                           0.0.0
- seq2seq                            1.0.0
- service-identity                   18.1.0
- setuptools                         40.8.0
- simplegeneric                      0.8.1
- singledispatch                     3.4.0.3
- six                                1.12.0
- smart-open                         1.8.0
- snowballstemmer                    1.2.1
- sortedcollections                  1.1.2
- sortedcontainers                   2.1.0
- soupsieve                          1.8
- spacy                              2.0.18
- Sphinx                             1.8.5
- sphinxcontrib-websupport           1.1.0
- spyder                             3.3.3
- spyder-kernels                     0.4.2
- SQLAlchemy                         1.3.1
- stats                              0.1.2a0
- statsmodels                        0.9.0
- subword-nmt                        0.3.6
- sympy                              1.3
- tables                             3.5.1
- tb-nightly                         1.14.0a20190301
- tblib                              1.3.2
- tensorboard                        1.13.1
- tensorboardX                       1.7
- tensorflow                         1.13.1
- tensorflow-estimator               1.13.0
- tensorflow-estimator-2.0-preview   1.14.0.dev2019052700
- tensorflow-gpu                     1.13.1
- tensorlayer                        2.0.2
- termcolor                          1.1.0
- terminado                          0.8.1
- testpath                           0.4.2
- tf-estimator-nightly               1.14.0.dev2019030115
- tf-nightly-gpu-2.0-preview         2.0.0.dev20190527
- tflearn                            0.3.2
- thinc                              6.12.1
- toolz                              0.9.0
- torch                              1.0.1.post2
- torchfile                          0.1.0
- torchtext                          0.3.1
- torchvision                        0.2.2.post3
- tornado                            6.0.2
- tqdm                               4.32.1
- traitlets                          4.3.2
- Twisted                            19.2.0
- typed-ast                          1.3.1
- typing                             3.6.4
- ujson                              1.35
- unicodecsv                         0.14.1
- urllib3                            1.24.1
- visdom                             0.1.8.8
- wcwidth                            0.1.7
- webencodings                       0.5.1
- websocket-client                   0.55.0
- Werkzeug                           0.14.1
- wheel                              0.33.1
- widgetsnbextension                 3.4.2
- winnowing                          0.1.2
- wrapt                              1.11.1
- wurlitzer                          1.0.2
- xgboost                            0.90
- xlrd                               1.2.0
- XlsxWriter                         1.1.5
- xlwt                               1.3.0
- xx-ent-wiki-sm                     2.0.0
- zict                               0.1.4
- zipp                               0.3.3
- zope.interface                     4.6.0
