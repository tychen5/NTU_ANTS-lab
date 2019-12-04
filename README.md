# Malware Analysis
*網路資訊安全實驗室*

## Malware Family Classification System based on Attention-based Characteristic Execution Sequence
* thesis paper: https://www.airitilibrary.com/Publication/alDetailedMesh1?DocID=U0001-1308201921554000#Summary
* source code detail in **seqgen_famclf/** directory

Working Set history slides: https://drive.google.com/drive/u/1/folders/1dTEAoiNkjQSOj-aknSAU32QE5a5xjWuM

Tokens level整理表: https://docs.google.com/spreadsheets/d/1myD3c_oEDJoF1ZUzFMnAX8LVpAe1V3mRi5Xmg7-HpXU/edit?fbclid=IwAR3UyE5122QFQ-c1cmQ-Shv63mdCZ03lA4DhwBXXAl7PftSHCh9w6nubJys#gid=780664369

We develop a neural network framework to learn malware representation based on invariant characteristics of a malware family. The main contributions:
*	We design an embedder, which uses BERT and Sent2Vec, state-of-the-art embedding modules, to capture relations within a single API call and among consecutive API calls in an execution trace.
*	We design an encoder which comprises gated recurrent units (GRU) to preserve the ordinal position of API calls and a self-attention mechanism for comparing intra-relations among different positions of API calls.
*	We visualize malware representation in a temporal space, demonstrating that the representation is explainable with physical meaning.
*	Results show that considering Sent2Vec to learn complete API call embeddings and GRU to explicitly preserve ordinal information yields more information and thus significant improvements. Also, the proposed approach effectively classifies new malicious execution traces on the basis of similarities with previously collected families.

