# 基於自注意力機制產生重要執行序的惡意程式家族分類系統 (Malware Family Classification System based on Attention-based Characteristic Execution Sequence)
**「自動化惡意程式重要執行序辨識系統」**


## Trained Model
* https://drive.google.com/drive/folders/1dzucL1MuvYtRaV2wevjVgD3_CIfJRDOD
    * \*.h5 : only parameters
    * \*_all.h5 : with architecture & model parameters
* system final model: byterep_0706_gruatt_sent2vec
* Embedder Experiment models: o2o_embEXP_gru_selfatt
* Encoder Experiment models: o2o_encEXP_gru_selfatt

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
* sample很靠近的是來自哪些family 0.9: 
