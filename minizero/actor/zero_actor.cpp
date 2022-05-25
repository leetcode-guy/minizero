#include "zero_actor.h"
#include "random.h"
#include "time_system.h"

namespace minizero::actor {

using namespace minizero;
using namespace network;

void MCTSSearchData::clear()
{
    selected_node_ = nullptr;
    node_path_.clear();
}

void ZeroActor::reset()
{
    BaseActor::reset();
    enable_resign_ = (utils::Random::randReal() < config::zero_disable_resign_ratio ? false : true);
}

void ZeroActor::resetSearch()
{
    BaseActor::resetSearch();
    mcts_.reset();
    mcts_search_data_.node_path_.clear();
}

Action ZeroActor::think(bool with_play /*= false*/, bool display_board /*= false*/)
{
    resetSearch();
    while (!isSearchDone()) { step(); }
    if (with_play) { act(getSearchAction()); }
    if (display_board) { displayBoard(); }
    return getSearchAction();
}

void ZeroActor::beforeNNEvaluation()
{
    mcts_search_data_.node_path_ = mcts_.select();
    if (alphazero_network_) {
        Environment env_transition = getEnvironmentTransition(mcts_search_data_.node_path_);
        nn_evaluation_batch_id_ = alphazero_network_->pushBack(env_transition.getFeatures());
    } else if (muzero_network_) {
        if (mcts_.getNumSimulation() == 0) { // initial inference for root node
            nn_evaluation_batch_id_ = muzero_network_->pushBackInitialData(env_.getFeatures());
        } else { // for non-root nodes
            const std::vector<MCTSNode*>& node_path = mcts_search_data_.node_path_;
            MCTSNode* leaf_node = node_path.back();
            MCTSNode* parent_node = node_path[node_path.size() - 2];
            assert(parent_node && parent_node->getExtraDataIndex() != -1);
            const std::vector<float>& hidden_state = mcts_.getTreeExtraData().getExtraData(parent_node->getExtraDataIndex()).hidden_state_;
            nn_evaluation_batch_id_ = muzero_network_->pushBackRecurrentData(hidden_state, env_.getActionFeatures(leaf_node->getAction()));
        }
    } else {
        assert(false);
    }
}

void ZeroActor::afterNNEvaluation(const std::shared_ptr<NetworkOutput>& network_output)
{
    const std::vector<MCTSNode*>& node_path = mcts_search_data_.node_path_;
    MCTSNode* leaf_node = node_path.back();
    if (alphazero_network_) {
        Environment env_transition = getEnvironmentTransition(node_path);
        if (!env_transition.isTerminal()) {
            std::shared_ptr<AlphaZeroNetworkOutput> alphazero_output = std::static_pointer_cast<AlphaZeroNetworkOutput>(network_output);
            mcts_.expand(leaf_node, calculateAlphaZeroActionPolicy(env_transition, alphazero_output));
            mcts_.backup(node_path, alphazero_output->value_);
        } else {
            mcts_.backup(node_path, env_transition.getEvalScore());
        }
    } else if (muzero_network_) {
        std::shared_ptr<MuZeroNetworkOutput> muzero_output = std::static_pointer_cast<MuZeroNetworkOutput>(network_output);
        mcts_.expand(leaf_node, calculateMuZeroActionPolicy(leaf_node, muzero_output));
        mcts_.backup(node_path, muzero_output->value_);
        leaf_node->setExtraDataIndex(mcts_.getTreeExtraData().store(MCTSNodeExtraData(muzero_output->hidden_state_)));
    } else {
        assert(false);
    }
    if (leaf_node == mcts_.getRootNode()) { addNoiseToNodeChildren(leaf_node); }
    if (isSearchDone()) { mcts_search_data_.selected_node_ = decideActionNode(); }
}

void ZeroActor::displayBoard() const
{
    assert(mcts_search_data_.selected_node_);
    const Action action = getSearchAction();
    std::cerr << env_.toString();
    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ")
              << "move number: " << env_.getActionHistory().size()
              << ", action: " << action.toConsoleString()
              << " (" << action.getActionID() << ")"
              << ", player: " << env::playerToChar(action.getPlayer()) << std::endl;
    std::cerr << "  root node info: " << mcts_.getRootNode()->toString() << std::endl;
    std::cerr << "action node info: " << mcts_search_data_.selected_node_->toString() << std::endl
              << std::endl;
}

void ZeroActor::setNetwork(const std::shared_ptr<network::Network>& network)
{
    assert(network);
    alphazero_network_ = nullptr;
    muzero_network_ = nullptr;
    if (network->getNetworkTypeName() == "alphazero") {
        alphazero_network_ = std::static_pointer_cast<AlphaZeroNetwork>(network);
    } else if (network->getNetworkTypeName() == "muzero") {
        muzero_network_ = std::static_pointer_cast<MuZeroNetwork>(network);
    } else {
        assert(false);
    }
    assert((alphazero_network_ && !muzero_network_) || (!alphazero_network_ && muzero_network_));
}

void ZeroActor::step()
{
    beforeNNEvaluation();
    if (alphazero_network_) {
        afterNNEvaluation(alphazero_network_->forward()[getNNEvaluationBatchIndex()]);
    } else if (muzero_network_) {
        if (mcts_.getNumSimulation() == 0) { // initial inference for root node
            afterNNEvaluation(muzero_network_->initialInference()[getNNEvaluationBatchIndex()]);
        } else { // for non-root nodes
            afterNNEvaluation(muzero_network_->recurrentInference()[getNNEvaluationBatchIndex()]);
        }
    } else {
        assert(false);
    }
}

MCTSNode* ZeroActor::decideActionNode()
{
    if (config::actor_select_action_by_count) {
        return mcts_.selectChildByMaxCount(mcts_.getRootNode());
    } else if (config::actor_select_action_by_softmax_count) {
        return mcts_.selectChildBySoftmaxCount(mcts_.getRootNode(), config::actor_select_action_softmax_temperature);
    }

    assert(false);
    return nullptr;
}

void ZeroActor::addNoiseToNodeChildren(MCTSNode* node)
{
    assert(node && node->getNumChildren() > 0);
    if (config::actor_use_dirichlet_noise) {
        const float epsilon = config::actor_dirichlet_noise_epsilon;
        std::vector<float> dirichlet_noise = utils::Random::randDirichlet(config::actor_dirichlet_noise_alpha, node->getNumChildren());
        MCTSNode* child = node->getFirstChild();
        for (int i = 0; i < node->getNumChildren(); ++i, ++child) {
            child->setPolicyNoise(dirichlet_noise[i]);
            child->setPolicy((1 - epsilon) * child->getPolicy() + epsilon * dirichlet_noise[i]);
        }
    } else if (config::actor_use_gumbel_noise) {
        std::vector<float> gumbel_noise = utils::Random::randGumbel(node->getNumChildren());
        MCTSNode* child = node->getFirstChild();
        for (int i = 0; i < node->getNumChildren(); ++i, ++child) {
            child->setPolicyNoise(gumbel_noise[i]);
            child->setPolicyLogit(child->getPolicyLogit() + gumbel_noise[i]);
        }
    }
}

std::vector<MCTS::ActionCandidate> ZeroActor::calculateAlphaZeroActionPolicy(const Environment& env_transition, const std::shared_ptr<network::AlphaZeroNetworkOutput>& alphazero_output)
{
    assert(alphazero_network_);
    std::vector<MCTS::ActionCandidate> action_candidates;
    for (size_t action_id = 0; action_id < alphazero_output->policy_.size(); ++action_id) {
        Action action(action_id, env_transition.getTurn());
        if (!env_transition.isLegalAction(action)) { continue; }
        action_candidates.push_back(MCTS::ActionCandidate(action, alphazero_output->policy_[action_id], alphazero_output->policy_logits_[action_id]));
    }
    sort(action_candidates.begin(), action_candidates.end(), [](const MCTS::ActionCandidate& lhs, const MCTS::ActionCandidate& rhs) {
        return lhs.policy_ > rhs.policy_;
    });
    return action_candidates;
}

std::vector<MCTS::ActionCandidate> ZeroActor::calculateMuZeroActionPolicy(MCTSNode* leaf_node, const std::shared_ptr<network::MuZeroNetworkOutput>& muzero_output)
{
    assert(muzero_network_);
    std::vector<MCTS::ActionCandidate> action_candidates;
    env::Player turn = (leaf_node == mcts_.getRootNode() ? env_.getTurn() : leaf_node->getAction().nextPlayer());
    for (size_t action_id = 0; action_id < muzero_output->policy_.size(); ++action_id) {
        const Action action(action_id, turn);
        if (leaf_node == mcts_.getRootNode() && !env_.isLegalAction(action)) { continue; }
        action_candidates.push_back(MCTS::ActionCandidate(action, muzero_output->policy_[action_id], muzero_output->policy_logits_[action_id]));
    }
    sort(action_candidates.begin(), action_candidates.end(), [](const MCTS::ActionCandidate& lhs, const MCTS::ActionCandidate& rhs) {
        return lhs.policy_ > rhs.policy_;
    });
    return action_candidates;
}

Environment ZeroActor::getEnvironmentTransition(const std::vector<MCTSNode*>& node_path)
{
    Environment env = env_;
    for (size_t i = 1; i < node_path.size(); ++i) { env.act(node_path[i]->getAction()); }
    return env;
}

} // namespace minizero::actor
