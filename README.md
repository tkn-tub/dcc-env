DCC-Env
=======

This is an environment for use with [Veins-Gym](https://www2.tkn.tu-berlin.de/software/veins-gym/).
The scenario uses DCC, the aim is to learn optimal parameters.
For this, the observations are the average channel busy time, reward is a metric derived from the vehicles' age of information, and the action are the CBR values used by DCC for its transition.

Initial steps:
- install dependencies of the simulation: [SUMO](https://www.eclipse.org/sumo/) (v1.6.0) and [OMNeT++](https://omnetpp.org/) (v5.6.*), such that you can run [Veins](http://veins.car2x.org/tutorial/) (v5.1, bundled with veins-gym)
- install the dependencies listed in `requirements.txt`
- build the simulation: `snakemake -jall`
- and run the example: `agents/trivial.py`.

For a deeper look into the simulation, see its configuration (`scenario`), and the `GymConnection` class.


Further Notes
=============

Check out [veins-gym](https://github.com/tkn-tub/veins-gym), which serves as a foundation for this work.
The veins-gym repository also contains a [Dockerfile](https://github.com/tkn-tub/veins-gym/blob/master/Dockerfile) that can be used to build a containerized environment to run the DCC-Env in.
