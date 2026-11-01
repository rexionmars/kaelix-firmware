"""Exporta a detecção por modo de falha em dado real decimado (MAFAULDA).

Usa o mesmo protocolo de training/kaelix_ml/train.py: cinco partições por arquivo, com
modelo e limiar ajustados em grupos diferentes. Não chama train(), que sobrescreveria
src/ml/isolation_forest_data.h.

Grava em experiments/figures/data/:
  tab_deteccao_classes.csv  detecção por classe e, nas de rolamento, por massa sobreposta
  tab_deteccao_limiar.csv   falso alarme com o limiar calibrado e com o padrão de 0,5

Uso:  uv run python experiments/figures/scripts/export_deteccao_data.py
"""
import re
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.model_selection import GroupKFold

RAIZ = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(RAIZ / "training"))

from kaelix_ml import dataset, train  # noqa: E402
from kaelix_ml.features import extract_features, features_to_vector  # noqa: E402

OUT = RAIZ / "experiments" / "figures" / "data"
LIMIAR_PADRAO = 0.5  # referência de interpretação do escore (Liu et al., 2008)

janelas = dataset.load_device_windows("mafaulda")
X = np.array([features_to_vector(extract_features(j.accel, j.sample_rate_hz)) for j in janelas])
y = np.array([j.label for j in janelas])
grupos = np.array([j.group for j in janelas])
classes = np.array([j.fault_class for j in janelas])
massa = np.array([m.group(1) if (m := re.search(r"/(\d+)g/", g)) else "" for g in grupos])

linhas = []
limiares = []
for parte, (tr, te) in enumerate(GroupKFold(n_splits=5).split(X, y, grupos)):
    modelo, limiar = train._fit_and_calibrate(X[tr], y[tr], grupos[tr], seed=train.RANDOM_STATE + parte)
    escore = train.device_score(modelo, X[te])
    alarme = escore > limiar
    normal = y[te] == "normal"
    limiares.append({"limiar": "calibrado", "parte": parte, "falso_alarme": alarme[normal].mean()})
    limiares.append({"limiar": "padrão", "parte": parte,
                     "falso_alarme": (escore[normal] > LIMIAR_PADRAO).mean()})
    for classe in np.unique(classes[te]):
        m = classes[te] == classe
        linhas.append({"classe": classe, "massa_g": "", "parte": parte,
                       "deteccao": alarme[m].mean(), "janelas": int(m.sum())})
        if classe.endswith("-bearing"):
            for nivel in np.unique(massa[te][m]):
                n = m & (massa[te] == nivel)
                linhas.append({"classe": classe, "massa_g": nivel, "parte": parte,
                               "deteccao": alarme[n].mean(), "janelas": int(n.sum())})

(pd.DataFrame(linhas)
 .groupby(["classe", "massa_g"], as_index=False)
 .agg(deteccao=("deteccao", "mean"), desvio=("deteccao", lambda v: v.std(ddof=0)), janelas=("janelas", "sum"))
 .to_csv(OUT / "tab_deteccao_classes.csv", index=False))

(pd.DataFrame(limiares)
 .groupby("limiar", as_index=False)
 .agg(falso_alarme=("falso_alarme", "mean"), desvio=("falso_alarme", lambda v: v.std(ddof=0)))
 .to_csv(OUT / "tab_deteccao_limiar.csv", index=False))

print(f"janelas {len(janelas)}, arquivos {len(set(grupos))}; CSVs em {OUT}")
