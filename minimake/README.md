# Minimake — correction commentée

Cette version couvre les fonctionnalités de base, les fonctionnalités avancées
et le bonus d'auto-compilation du sujet. Les documents fournis servent de
spécification du programme et de référence de style.

## Construire et vérifier

Depuis ce répertoire, sous Linux ou WSL :

```sh
make
make check
make check-sanitize
make check-format
make clean
```

La compilation utilise `-std=c99 -pedantic -Werror -Wall -Wextra -Wvla`.
`make check` lance les tests unitaires Criterion puis les tests fonctionnels
POSIX shell. Les dépendances sont un compilateur C, GNU make, pkg-config et
Criterion (`libcriterion-dev` sous Ubuntu). `clang-format` sert uniquement aux
cibles de formatage. Les tests fonctionnels utilisent aussi les outils Unix
usuels et Python 3 pour vérifier les dates à la nanoseconde.

`make check-unit` et `make check-functional` permettent de lancer les suites
séparément. `make check-sanitize` exécute les tests fonctionnels avec
AddressSanitizer, UndefinedBehaviorSanitizer et la détection des fuites mémoire.

```sh
BIN=/chemin/absolu/minimake sh tests/tests.sh
TEST_FILTER='pattern*' sh tests/tests.sh
KEEP_TMP=1 sh tests/tests.sh
```

Chaque cas fonctionnel dispose d'un répertoire temporaire distinct. Les tests
comparent les sorties, les codes de retour et les fichiers produits. Ils fixent
les dates explicitement, sans dépendre de pauses entre deux commandes.

## Architecture

| Module dans `src/` | Responsabilité |
| --- | --- |
| `main.c` | Orchestrer les phases et libérer les ressources. |
| `options.c` | Lire les options, leurs arguments et les cibles demandées. |
| `load.c`, `remake.c` | Charger, reconstruire et relire les makefiles. |
| `parser.c` | Reconnaître définitions, dépendances et recettes. |
| `expand.c` | Développer les références et détecter les cycles de variables. |
| `model.c`, `variables.c`, `strings.c` | Posséder et manipuler les données. |
| `pretty.c` | Produire la représentation demandée par `-p`. |
| `pattern.c` | Choisir et instancier les règles contenant `%`. |
| `build.c` | Parcourir les dépendances et décider quoi reconstruire. |
| `execute.c` | Journaliser et exécuter chaque commande avec `/bin/sh -c`. |
| `memory.c`, `text.c` | Centraliser allocations et opérations sur les chaînes. |

Le modèle possède ses chaînes et ses listes : les fonctions d'ajout copient
leurs arguments, et `destroy_makefile` libère l'ensemble. `expand_string`
renvoie une chaîne allouée à libérer, ou `NULL` après un diagnostic. Les
opérations susceptibles d'échouer renvoient `0` ou `2`.

Le parseur ne lance aucune commande. Il développe immédiatement les noms de
variables, les cibles et toute la liste des dépendances, puis découpe cette
dernière. Les valeurs de variables et les recettes restent brutes jusqu'à leur
utilisation. Cette séparation permet les références à une variable déclarée
plus loin dans le fichier et un pretty-print fidèle.

Le moteur distingue l'état de visite d'une règle et le fait que sa construction
a modifié une dépendance. Une règle commune à deux branches est exécutée une
fois, mais sa modification se propage aux deux parents pendant le parcours.
Les diagnostics « up to date » et « Nothing to be done » concernent les cibles
demandées, sans bruit pour les dépendances intermédiaires.

## Comportements à retenir

- Les espaces au début des valeurs et des commandes sont retirés ; les espaces
  en fin de ligne sont conservés. Les lignes vides n'ajoutent pas de recette.
- Une variable contenant plusieurs dépendances est développée avant de découper
  les mots. Les formes `$(VAR)`, `${VAR}`, `$V`, `$$`, les noms imbriqués et les
  valeurs récursives sont pris en charge.
- L'environnement sert de repli. Dans les recettes, `$@`, `$<` et `$^` décrivent
  la cible courante ; `$^` conserve l'ordre en éliminant les doublons.
- Les dates sont comparées à la nanoseconde. Une dépendance plus récente doit
  reconstruire une cible existante, même sans recette propre.
- `.PHONY` ignore le fichier du même nom et conserve la déduplication pendant
  une invocation. La déclaration `.PHONY` n'est pas une cible par défaut.
- Une règle explicite prime sur les motifs. Le motif donnant le radical le plus
  court gagne, puis l'ordre de définition départage les égalités. Seul le premier
  `%` est spécial ; une instance conserve ses caractères `%` littéraux.
- La fonctionnalité avancée des `-f` multiples charge tous les fichiers, dans
  leur ordre, et remplace le comportement de base « dernier fichier seulement ».
  Les fichiers manquants sont recherchés comme cibles avant les objectifs
  explicites. La reconstruction se fait dans l'ordre inverse des `-f`, comme
  GNU make. Un makefile effectivement modifié est relu.
- `-f -` lit l'entrée standard et en conserve une copie temporaire pour les
  relectures. `-f -h` désigne un fichier nommé `-h`.
- `-p` n'exécute aucune recette, y compris celles reconstruisant un makefile.
  Un fichier manquant reste donc une erreur dans ce mode.

Les continuations avec `\`, les cibles multiples, les opérateurs d'affectation
autres que `=`, les fonctions GNU make et les recettes sur la ligne d'une règle
ne font pas partie de la syntaxe demandée. Les cycles de dépendances et de
variables reçoivent un diagnostic ; les relectures de makefiles sont limitées
à 64 pour interrompre une reconstruction qui ne se stabilise jamais.

## Auto-compilation

`Minimakefile` utilise exclusivement la syntaxe implémentée :

```sh
make
cp minimake /tmp/minimake-bootstrap
make clean
/tmp/minimake-bootstrap -f Minimakefile
./minimake -f Minimakefile
```

Le premier passage reconstruit le programme, le second constate qu'il est à
jour. Le test fonctionnel correspondant travaille sur une copie isolée du
projet, puis vérifie également la cible `clean`.

## Points corrigés par rapport à la version initiale

Le fichier unique mélangeait stockage, parsing, options et exécution. Une ligne
vide pouvait devenir une commande vide en tête de recette et empêcher toute la
construction. Les espaces finaux des variables disparaissaient, les dépendances
issues d'une variable étaient découpées avant expansion, et une dépendance
fichier plus récente pouvait laisser sa cible inchangée. La sélection de `-f`,
les messages de statut et plusieurs fonctionnalités avancées étaient également
incomplets. Les tests couvrent ces régressions et les frontières entre modules.

Les fichiers texte utilisent LF. `.gitattributes` impose LF lors des prochains
checkouts et `.editorconfig` indique ce format aux éditeurs. Le code suit la
limite exceptionnelle de 40 lignes par corps de fonction du sujet, avec au plus
quatre paramètres, sans variable globale, cast explicite ou `goto`. Les règles
de style sont vérifiées localement ; la moulinette EPITA n'est pas disponible
dans ce dépôt.
