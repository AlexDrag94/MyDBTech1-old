//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H


#include <memory>
#include <cmath>
#include "SimpleGraph.h"
#include "RPQTree.h"
#include "Evaluator.h"
#include "Graph.h"

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

public:

    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEvaluator() = default;

    void prepare() override ;
    cardStat evaluate(RPQTree *query) override ;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> evaluate_aux(RPQTree *q);
    static std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g);
    static std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> join(std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &left, std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &right);

    std::vector<RPQTree*> find_leaves(RPQTree *query);
    RPQTree* best = nullptr;
    RPQTree* query_optimizer(RPQTree *query);
    void query_optimizer2(std::vector<RPQTree*> query, uint32_t sum);


    static cardStat computeStats(std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &g);

};


#endif //QS_SIMPLEEVALUATOR_H
