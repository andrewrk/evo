#ifndef GENETICALGORITHM_H
#define GENETICALGORITHM_H


/*
    Usage:

    GeneticAlgorithm * ga = new GeneticAlgorithm(your_ga_client);
    ga->computeFirstGeneration();
    for (int i = 0; i < 10; i++) {
        ga->computeNextGeneration();
    }
    ga->

  */
class GeneticAlgorithmClient;

class GeneticAlgorithm
{
public:
    GeneticAlgorithm();

    void computeFirstGeneration();
    void computeNextGeneration();
};

#endif // GENETICALGORITHM_H
