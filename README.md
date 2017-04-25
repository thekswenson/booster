# BOOSTER : BOOtstrap Support by TransfER
This tool is dedicated to compute bootstrap supports using transfer distance.

# Help

## Dependencies
BOOSTER depends on [OpenMP](https://fr.wikipedia.org/wiki/OpenMP). 
You should first install it before building BOOSTER.

Under ubuntu for example:
```
apt-get install libgomp1
```

## Install BOOSTER

* First download a [release](https://github.com/fredericlemoine/booster/releases) or clone the repository;
* Then enter the directory and type `make`
* The booster executable should be located in the current directory

## Usage

```
Usage: booster -i <tree file> -b <bootstrap prefix or file> [-r <# rand shufling> -n <normalization> -@ <cpus> -s <seed> -S <stat file> -o <output tree> -v]
Options:
      -i : Input tree file
      -b : Bootstrap tree file (1 file containing all bootstrap trees)
      -o : Output file (optional), default : stdout
      -@ : Number of threads (default 1)
      -a, --algo  : bootstrap algorithm, tbe or fbp (default tbe)
      -S : Prints output statistics for each branch in the given output file
      -q, --quiet : Does not print progress messages during analysis
      -v : Prints version (optional)
      -h : Prints this help
```

## Options
* `-i`: Reference tree file : a reference tree in newick format
* `-b`: Bootstrap tree file : a set of bootstrap trees in newick format
* `-@`: Number of threads
* `-a`: Bootstrap algorithm: `tbe` (Transfer bootstrap) or `fbp` (Felsenstein bootstrap)
* `-S`: Output statistic file

## Example of workflow

You have a nucleotide alignment and you want to compute booster supports with 100 bootstrap samples. The first step is to generate reference and bootstrap trees. Several ways to do it depending on the phylogenetic tool you want to use:

* PhyML: PhyML already generates bootstrap trees
```bash
# Compute trees
phyml -i align.phy -d nt -b 100 -m GTR -f e -t e -c 6 -a e -s BEST -o tlr 
# Compute booster supports
booster -a tbe -i align.phy_phyml_tree.txt -b align.phy_phyml_boot_trees.txt -@ 5 -o booster.nw
```

* RAxML: FBP with RAxML
```bash
# Build reference tree
raxmlHPC -m GTRGAMMA -p $RANDOM -s align.phy -n REF
# Build bootstrap trees
raxmlHPC -m GTRGAMMA -p $RANDOM -b $RANDOM -# 100 -s align.phy -n BOOT
# Compute booster support
booster -a tbe -i RAxML_bestTree.REF -b RAxML_bootstrap.BOOT -@ 5 -o booster.nw
```

* RAxML: Booster supports and rapid bootstrap
```bash
# Build reference tree + bootstrap trees
raxmlHPC -f a -m GTRGAMMA -c 4 -s align.phy -n align -T 4 -p $RANDOM -x $RANDOM -# 100
# Compute booster support
booster -a tbe -i RAxML_bestTree.align -b RAxML_bootstrap.align -@ 5 -o booster.nw
```

* FastTree: You will need to generate bootstrap alignments (Phylip format), with [goalign](https://github.com/fredericlemoine/goalign) for example
```bash
# Build bootstrap alignments
goalign build seqboot -i align.phy -p -n 100 -o boot -S
cat boot*.ph > boot.phy
# Build reference tree
FastTree -nt -gtr align.phy > ref.nhx
# Build bootstrap trees
FastTree -nt -n 100 -gtr boot.phy > boot.nhx
# Compute booster supports
booster -a tbe -i ref.nhx -b boot.nhx -@ 5 -o booster.nw
```

* IQ-TREE : Booster supports and ultrafast bootstrap
```
# Infer ML tree + ultrafast bootstrap trees
iqtree-omp -wbt -s align.phy -m GTR -bb 100 -nt 5
# Compute booster supports
booster -a tbe -i align.phy.treefile -b align.phy.ufboot -@ 5 -o booster.nw
```
