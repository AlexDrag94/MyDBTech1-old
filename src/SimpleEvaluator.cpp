//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &g) {

    return {0, g->size(), 0};
}

std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<std::vector<std::pair<uint32_t,uint32_t>>>();

    if(in->adj.empty()) {
        return out;
    }

    for (const auto &edge : in->adj[projectLabel]) {
        if(!inverse)
            out->emplace_back(edge.first, edge.second);
        else
            out->emplace_back(edge.second, edge.first);
    }

    return out;
}

std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> SimpleEvaluator::join(std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &left, std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> &right) {

    auto out = std::make_shared<std::vector<std::pair<uint32_t,uint32_t>>>();

    if(left->empty() || right->empty()) {
        return out;
    }

    std::sort(right->begin(), right->end(), SimpleGraph::sortPairsFirst);
    std::vector<uint32_t> pos;
    pos.resize(right->back().first + 1);

    for(uint32_t i = 0; i < pos.size(); i ++) {
        pos[i] = right->size();
    }

    for(uint32_t j = 0; j < right->size(); j ++) {
        if(j < pos[right->at(j).first])
            pos[right->at(j).first] = j;
    }

    for(uint32_t i = 0; i < left->size(); i ++) {
        if(left->at(i).second < pos.size()) {
            for (uint32_t j = pos[left->at(i).second]; j < right->size(); j++) {
                if (out->empty()) {
                    if (left->at(i).second == right->at(j).first)
                        out->emplace_back(left->at(i).first, right->at(j).second);
                    else break;
                } else {
                    if (left->at(i).second != right->at(j).first) {
                        break;
                    } else if (left->at(i).first != out->back().first || right->at(j).second != out->back().second)
                        out->emplace_back(left->at(i).first, right->at(j).second);
                }
            }
        }
    }
    if(!out->empty()) {
        std::sort(out->begin(), out->end());
        out->erase(unique(out->begin(), out->end()), out->end());
    }
    return out;
}

std::shared_ptr<std::vector<std::pair<uint32_t,uint32_t>>> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEvaluator::project(label, inverse, graph);
    }

    else if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);

    }

    return nullptr;
}


std::vector<RPQTree*> SimpleEvaluator::find_leaves(RPQTree *query) {
    std::vector<RPQTree*> final;
    if (query->isLeaf()) {
        return {query};
    }
    else
    {
        if (query->right) {
            auto process = find_leaves(query->right);
            final.insert(final.begin(), process.begin(), process.end());
        }
        if (query->left)
        {
            auto process = find_leaves(query->left);
            final.insert(final.begin(), process.begin(), process.end());
        }

    }
    return final;
}

RPQTree* SimpleEvaluator::query_optimizer(RPQTree *query) {
    std::vector<RPQTree*> leaves = find_leaves(query);

    while (leaves.size() > 1) {
        uint32_t bestScore;
        bool first = true;
        RPQTree *bestTree = nullptr;
        int index = -1;

        for (int i = 0; i < leaves.size()-1; ++i) {
            std::string data("/");
            auto *currentTree = new RPQTree(data, leaves[i], leaves[i+1]);
            uint32_t currentScore = est->estimate(currentTree).noPaths;
            if(first) {
                bestScore = currentScore;
                bestTree = currentTree;
                index = i;
                first = false;
            }
            else if (bestScore > currentScore) {
                bestScore = currentScore;
                bestTree = currentTree;
                index = i;
            }
            if(bestScore < 1) {
                break;
            }
        }

        leaves.erase(leaves.begin() + index + 1);
        leaves[index] = bestTree;
    }

    return leaves[0];
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    RPQTree* res_query = query_optimizer(query);
    res_query->print();

    auto res = evaluate_aux(res_query);
    return SimpleEvaluator::computeStats(res);
}