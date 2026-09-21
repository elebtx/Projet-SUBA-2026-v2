## 1. Génération des évènements avec MadGraph

Lancez MadGraph depuis votre terminal (version 2.9.27 ici) et exécutez les commandes suivantes pour générer le processus $pp \rightarrow Z \rightarrow \mu^+ \mu^-$ au format LHE :

```text
generate p p > z > mu+ mu-
output ppZmumu
launch
```
à la suite de la dernière commande valider deux fois les choix par défaut en tapant ENTER.

Pour modifier les paramètres physique, rendez-vous dans le ficher `run_card.dat` (il se trouve dans votre dossier `ppZmumu` crée lors de l'étape précédente). Afin de réaliser les mêmes histogrammes, il vous faut lancer deux runs :
- **Run 01 :** $N_{events}= 10000$ et $E_{beam1} = E_{beam2} = 6500$ GeV (il s'agit du run de référence avec les paramètres par défaut de MadGraph);
- **Run 02 :** Changez $E_{beam2} =  1000$ GeV.

> Note : D'autres Run ont également été réalisés pour ce projet, le Run 02 est donné ici à titre d'exemple.

Après avoir modifié et enregistré le fichier `run_card.dat` comme indiqué, il n'est pas utile de relancer MadGraph, il suffit de taper la commande suivante :

```text
bin/generate_events
```

Pour chaque run, les événements sont générés sous forme d'archive `Events/run_01/unweighted_events.lhe.gz` et `Events/run_02/unweighted_events.lhe.gz`. Décompressez ces fichiers manuellement ou avec la commande :

```text
gunzip unweighted_events.lhe.gz
```

---

## 2. Conversion des fichiers LHE en arbres ROOT

Utilisez le script `lhe2root.C` pour convertir les fichiers LHE en arbre ROOT. Depuis votre terminal, lancez ROOT (version  6.40.04) :

```bash
root -l
```

et exécutez la macro `lhe2root.C` :

```cpp
.x lhe2root.C("ppZmumu/Events/run_01/unweighted_events.lhe", "nominal.root", -1)
.x lhe2root.C("ppZmumu/Events/run_02/unweighted_events.lhe", "asymetrique.root", -1)
```

> **N.B. :** Assurez vous que votre fichier `lhe2root.C` se trouve bien dans votre répertoire de travail.

---

## 3. Exécution de l'analyse

Le filtrage cinématique, le calcul des efficacités et les fits sont réalisés par la classe `Analyze`. Dans ROOT, ouvrez le fichier de données et lancez l'analyse :

```cpp
TFile f("nominal.root");
Events->Process("Analyze.C");
```

> **N.B. :** Avant de commencer cette étape, assurez vous que vos fichiers `nominal.root`, `asymetrique.root`, `AnalyseZ.C` et `AnalyseZ.h` soient dans votre répertoire de travail.*

L'exécution de cette commande affiche dans le terminal :
- le nombre d'évènements ($N_{tot}$); 
- le nombre d'évènements retenus après la sélection ($N_{pass}$);
- l'efficacité globale ($\varepsilon = N_{pass}/N_{tot}$);
- l'incertitude binomiale;
- l'incertitude de Clopper-Pearson.

Elle génère également les figures suivantes :
- l'efficacité différentielle en fonction de la pseudo-rapidité $\eta$;
- le fit de la masse invariante $m_{\mu\mu}$ par une fonction de Breit-Wigner relativiste;

Pour réaliser cette même analyse avec les données du Run 02, il suffit de reproduire ces mêmes étapes avec le fichier `asymetrique.root`
