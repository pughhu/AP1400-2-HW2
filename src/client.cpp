#include "client.h"
#include "crypto.h"
#include <memory>
#include <map>
#include <random>

std::default_random_engine rd_engine;
// std::mt19937 rd_engine(std::random_device{}());
class Server
{
public:
	Server();
	std::shared_ptr<Client> add_client(std::string id);
	std::shared_ptr<Client> get_client(std::string id)const;
	double get_wallet(std::string id)const;
	static bool parse_trx(std::string& trx, std::string& sender, std::string& receiver, double& value);
	bool add_pending_trx(std::string trx, std::string signature)const;
	size_t mine();
    friend void show_wallets(const Server& server);
private:
	std::map<std::shared_ptr<Client>, double> clients;
};

Client::Client(std::string id, const Server& server):server{&server}, id{id}{
    // this->server = &server;
    // std::string public_key{}, private_key{};
    crypto::generate_key(public_key, private_key);
}

std::string Client::get_id(){
    std::string x = id;
    return x;
}

std::string Client::get_publickey()const{
    return public_key;
}

double Client::get_wallet(){
    return server->get_wallet(id);
}

std::string Client::sign(std::string txt)const{
    std::string signature = crypto::signMessage(private_key, txt);
    return signature;
}

bool Client::transfer_money(std::string receiver, double value){
    std::string trx = Client::get_id().append("-").append(receiver).append("-").append(std::to_string(value));
    std::string signature = Client::sign(trx);
    auto result = server->add_pending_trx(trx, signature);
    return result;
}

size_t Client::generate_nonce(){
    // std::random_device rd_engine{};
    // std::default_random_engine rd_engine;
    std::uniform_int_distribution<int> int_d{0,std::numeric_limits<int>::max()};
    return int_d(rd_engine);
}