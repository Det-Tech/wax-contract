#include "spaceminer.hpp"

    void space_contract::setenv(uint32_t row, uint32_t column, const asset& click_price, const asset& grand_prize,
                                const asset& large_prize, const asset& medium_prize, const asset& small_prize,
                                bool inGame, uint64_t smallChance, uint64_t mediumChance, uint64_t largeChance,
                                uint64_t grandChance){
        require_auth(_self);

        space_env env = space_env();

        env.row = row;
        env.column = column;
        env.click_price = click_price;
        env.grand_prize = grand_prize;
        env.large_prize = large_prize;
        env.medium_prize = medium_prize;
        env.small_prize = small_prize;
        env.inGame = inGame;

        space_env_singleton _env(get_self(), get_self().value);

        _env.set(env, get_self());

        space_env_2 env_2 = space_env_2();
        env_2.smallChance = smallChance;
        env_2.mediumChance = mediumChance;
        env_2.largeChance = largeChance;
        env_2.grandChance = grandChance;

        space_env_singleton_2 _env_2(get_self(), get_self().value);
        
        _env_2.set(env_2, get_self());

        map_state mstate = map_state();

        int r = (int) row;
        int c = (int) column;

        vector<uint8_t> v(r*c, (uint8_t) 1);
        mstate.map = v;

        map_singleton _map(get_self(), get_self().value);

        _map.set(mstate, get_self());
    }

    void space_contract::transfer(uint64_t sender, uint64_t receiver){
        auto transfer_data = unpack_action_data<st_transfer>();

	    if(transfer_data.from == _self){
		return;
	    }

        check(transfer_data.from == _self || transfer_data.to == _self, "Invalid transfer");

        check(transfer_data.quantity.is_valid(), "Invalid asset");

        space_env_singleton _env(get_self(), get_self().value);

        auto space_env = _env.get();

        check(space_env.click_price.amount <= transfer_data.quantity.amount, "Invalid transfer amount");

        player_table _players(get_self(), get_self().value);

        auto itr = _players.find(transfer_data.from.value);

        uint32_t clicks_purchased = transfer_data.quantity.amount / space_env.click_price.amount;

        if(transfer_data.quantity.amount == space_env.click_price.amount * 9){
            clicks_purchased++;
        }
        else if(transfer_data.quantity.amount == space_env.click_price.amount * 45){
            clicks_purchased += 5;
        }

        if(itr == _players.end()){
            _players.emplace(_self, [&](auto& player) {
                player.account = transfer_data.from;
                player.clicks += clicks_purchased;
            });
        }
        else{
            _players.modify(itr, same_payer, [&](auto& player){
                player.clicks += clicks_purchased;
            });
        }
    }

    void space_contract::click(name account, uint32_t row, uint32_t column) {
        require_auth(account);

        space_env_singleton _env(get_self(), get_self().value);
        auto space_env = _env.get();

        check(space_env.inGame, "Game is not live");

        check(row >= 0 && row < space_env.row, "row out of range");

        check(column >= 0 && column < space_env.column, "column out of range");

        player_table _players(get_self(), get_self().value);

        auto itr = _players.find(account.value);

        check(itr != _players.end(), "player has no clicks");

        check((*itr).clicks > 0, "player has no clicks");

        if((*itr).clicks > 1){
            _players.modify(itr, same_payer, [&](auto& player){
                player.clicks--;
            });
        }
        else{
            _players.erase(itr);
        }

        map_singleton _map(get_self(), get_self().value);

        auto mstate = _map.get();

        vector<uint8_t> v = mstate.map;

        int r = (int) row;
        int c = (int) column;
        int tr = (int) space_env.row;
        int tc = (int) space_env.column;

        check(v[r * tc + c] == (uint8_t) 1, "This location has already been guessed.");

        v[r * tc + c] = (uint8_t) 0;
        mstate.map = v;
        _map.set(mstate, get_self());

        uint64_t customer_id = account.value;
        uint64_t tx_signing_value = generateSign(account.value);

        //now that you've generated a unique signing_value, call the WAX RNG action.
        eosio::action(
            eosio::permission_level{ get_self() , "active"_n },
            "orng.wax"_n, "requestrand"_n,
            std::make_tuple( customer_id, tx_signing_value, get_self() )
        ).send();
    }

    void space_contract::receiverand(uint64_t customer_id, const eosio::checksum256& random_value){
        
        auto byte_array = random_value.extract_as_byte_array();

        space_env_singleton_2 _env_2(get_self(), get_self().value);
        auto space_env_2 = _env_2.get();

        //cast the random_value to a smaller number
        uint64_t max_value = 300;
        uint64_t small = space_env_2.mediumChance + space_env_2.largeChance + space_env_2.grandChance;
        uint64_t medium = space_env_2.largeChance + space_env_2.grandChance;
        uint64_t large = space_env_2.grandChance;
        uint64_t grand = 0;

        uint64_t random_int = 0;
        for (int i = 0; i < 8; i++) {
            random_int <<= 8;
            random_int |= (uint64_t)byte_array[i];
        }

        uint64_t num1 = random_int % max_value;

        space_env_singleton _env(get_self(), get_self().value);
        auto space_env = _env.get();

        if(num1 >= grand && num1 < large){
            eosio::action(
                eosio::permission_level{get_self() , "active"_n },
                "eosio.token"_n, "transfer"_n,
                std::make_tuple( get_self(), name(customer_id), space_env.grand_prize, std::string("Congratulations! You have found the GRAND treasure."))
            ).send();

            space_env.round += 1;
            space_env.inGame = false;
            _env.set(space_env, get_self());
        }
        else if(num1 >= large && num1 < medium){
            eosio::action(
                eosio::permission_level{get_self() , "active"_n },
                "eosio.token"_n, "transfer"_n,
                std::make_tuple( get_self(), name(customer_id), space_env.large_prize, std::string("Congratulations! You have found a LARGE treasure."))
            ).send();
        }
        else if(num1 >= medium && num1 < small){
            eosio::action(
                eosio::permission_level{get_self() , "active"_n },
                "eosio.token"_n, "transfer"_n,
                std::make_tuple( get_self(), name(customer_id), space_env.medium_prize, std::string("Congratulations! You have found a MEDIUM treasure."))
            ).send();
        }
        else if(num1 >= small && num1 < (space_env_2.smallChance + space_env_2.mediumChance + space_env_2.largeChance + space_env_2.grandChance)){
            eosio::action(
                eosio::permission_level{get_self() , "active"_n },
                "eosio.token"_n, "transfer"_n,
                std::make_tuple( get_self(), name(customer_id), space_env.small_prize, std::string("Congratulations! You have found a SMALL treasure."))
            ).send();
        }
    }

    void space_contract::restart(){
	require_auth(_self);

        space_env_singleton _env(get_self(), get_self().value);

        auto space_env = _env.get();

        map_state mstate = map_state();

        int r = (int) space_env.row;
        int c = (int) space_env.column;

        vector<uint8_t> v(r*c, (uint8_t) 1);
        mstate.map = v;

        map_singleton _map(get_self(), get_self().value);

        _map.set(mstate, get_self());

        space_env.inGame = true;

        _env.set(space_env, get_self());
    }

    uint64_t space_contract::generateSign(uint64_t i){
        space_env_singleton _env(get_self(), get_self().value);
        auto space_env = _env.get();

        time_point_sec current_time = current_time_point();

        init_genrand64((space_env.round + current_time.sec_since_epoch()) + i);
        uint64_t rand = genrand64_int64();

        return rand;
    }


#define EOSIO_DISPATCH_CUSTOM( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == "eosio.token"_n.value || code == "orng.wax"_n.value ) { \
         if( action == "transfer"_n.value){ \
           check( code == "eosio.token"_n.value, "Must transfer WAX"); \
         } \
         switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_DISPATCH_CUSTOM(space_contract, (setenv)(transfer)(click)(receiverand)(restart))
