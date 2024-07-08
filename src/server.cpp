#include "server.h"
#include "crypto.h"
#include <random>
#include <map>
#include <regex>
#include <stdexcept>

std::vector<std::string> pending_trxs;
class Client
{
public:
	Client(std::string id, const Server& server);
	std::string get_id();
	std::string get_publickey()const;
	double get_wallet();
	std::string sign(std::string txt)const;
	bool transfer_money(std::string receiver, double value);
	size_t generate_nonce();
private:
	Server const* const server;
	const std::string id;
	std::string public_key;
	std::string private_key;
};

Server::Server(){
    std::map<std::shared_ptr<Client>, double> clients {};
}

std::shared_ptr<Client> Server::add_client(std::string id){
    for(auto it=clients.begin(); it!=clients.end(); ++it){
        if((*it).first->get_id() == id){
            std::random_device rd_engine{};
            std::uniform_int_distribution<int> int_d{1000, 9999};
            auto x = int_d(rd_engine);
            id += std::to_string(x);
            break;
        }
    }
    auto ptr = std::make_shared<Client>(id, *this);
    // clients.emplace(ptr, 5);
    clients.insert({ptr, 5});
    return ptr;
}

std::shared_ptr<Client> Server::get_client(std::string id)const{
    for(auto it=clients.begin(); it!=clients.end(); ++it){
        if((*it).first->get_id() == id){
            return (*it).first;
        }
    }
    // return std::shared_ptr<Client>{};
    return nullptr;
}

double Server::get_wallet(std::string id)const{
    auto ptr = Server::get_client(id);
    auto ele = clients.find(ptr);
    if(ele == clients.end())
        throw std::logic_error("no sucn id.");
    return (*ele).second;
}

bool Server::parse_trx(std::string& trx, std::string& sender, std::string& receiver, double& value){
    std::regex regex{R"(([a-zA-Z_]+)-([a-zA-Z_]+)-(\d+(\.\d+)?))"};
    std::smatch details{};
    auto matched = std::regex_match(trx, details, regex);
    if(matched){
        sender = details[1];
        receiver = details[2];
        value = std::stod(details[3]);
    }else{
        throw std::runtime_error("The string is not standard.");
    }
    
    return matched;
}

bool Server::add_pending_trx(std::string trx, std::string signature)const{
    std::string sender{}, receiver{};
    double value{};
    auto matched = Server::parse_trx(trx, sender, receiver, value);

    auto sender_ptr = Server::get_client(sender);
    auto receive_ptr = Server::get_client(receiver);
    if(receive_ptr == nullptr){
        // std::cout<<"receive_ptr = nullptr"<<std::endl;
        return false;
    }

    bool authentic = crypto::verifySignature(sender_ptr->get_publickey(), trx, signature);
    bool enough = sender_ptr->get_wallet() > value;

    bool success = enough & authentic;
    if(success){
        pending_trxs.push_back(trx);
    }
    // std::cout<<"receive_ptr != nullptr"<<std::endl;
    return success;
}

size_t Server::mine(){
    std::string trx_all{};
    for(const auto& item : pending_trxs){
        trx_all += item;
    }

    // debug
    std::cout<<"Debug: "<<trx_all<<std::endl;

    bool success = false;
    size_t nonce;
    std::shared_ptr<Client> ptr_miner;

    for(auto it=clients.begin();!success;++it){
        if(it == clients.end())
            it = clients.begin();
        // 
        nonce = (*it).first->generate_nonce();
        std::string trx_all_nonce = trx_all + std::to_string(nonce);
        // debug
        
        std::string hash{crypto::sha256(trx_all_nonce)};
        //
        std::string hash_ = std::string(hash,0,10);
        // std::string hash_ = hash.substr(0,10);
        std::regex regex{"000"};
        success = std::regex_search(hash_, regex);
        // if(hash_.find("000") != std::string::npos){
        //     success = true;
        //     std::cout<<"success!"<<std::endl;
        // }
        if(success){
            ptr_miner = (*it).first;
            break;
        }
    }
    std::cout<<"The id of the miner: "<<ptr_miner->get_id()<<std::endl;
    double old_value = clients[ptr_miner];
    clients.insert_or_assign(ptr_miner,old_value+6.25);

    for(auto item: pending_trxs){
        std::string sender;
        std::string receiver;
        double value;
        Server::parse_trx(item, sender, receiver, value);
        auto id_sent = Server::get_client(sender);
        auto id_receive = Server::get_client(receiver);
        clients.insert_or_assign(id_sent, clients[id_sent]-value);  
        clients.insert_or_assign(id_receive, clients[id_receive]+value); 
    }

    pending_trxs.clear();
    return nonce;
}