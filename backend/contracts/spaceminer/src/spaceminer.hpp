/*
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and
     Computer Simulation 8. (Jan. 1998) 3--30.
*/

#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/singleton.hpp>
#include <eosio/contract.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/time.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

#include <stdio.h>

    using std::string;
    using std::vector;
    using std::set;

    using eosio::asset;
    using eosio::transaction;
    using eosio::permission_level;
    using eosio::action;
    using eosio::name;
    using eosio::unpack_action_data;
    using eosio::transaction;
    using eosio::datastream;
    using eosio::block_timestamp;
    using eosio::time_point;
    using eosio::microseconds;
    using eosio::check;
    using eosio::same_payer;
    using eosio::time_point_sec;
    using eosio::current_time_point;

    #define NN 312
    #define MM 156
    #define MATRIX_A 0xB5026F5AA96619E9ULL
    #define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
    #define LM 0x7FFFFFFFULL /* Least significant 31 bits */

    /* The array for the state vector */
    static unsigned long long mt[NN];
    /* mti==NN+1 means mt[NN] is not initialized */
    static int mti=NN+1;

    /* initializes mt[NN] with a seed */
    void init_genrand64(unsigned long long seed)
    {
        mt[0] = seed;
        for (mti=1; mti<NN; mti++)
            mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
    }

    unsigned long long genrand64_int64(void)
    {
        int i;
        unsigned long long x;
        static unsigned long long mag01[2]={0ULL, MATRIX_A};

        if (mti >= NN) { /* generate NN words at one time */

            /* if init_genrand64() has not been called, */
            /* a default initial seed is used     */
            if (mti == NN+1)
                init_genrand64(5489ULL);

            for (i=0;i<NN-MM;i++) {
                x = (mt[i]&UM)|(mt[i+1]&LM);
                mt[i] = mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
            }
            for (;i<NN-1;i++) {
                x = (mt[i]&UM)|(mt[i+1]&LM);
                mt[i] = mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
            }
            x = (mt[NN-1]&UM)|(mt[0]&LM);
            mt[NN-1] = mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

            mti = 0;
        }

        x = mt[mti++];

        x ^= (x >> 29) & 0x5555555555555555ULL;
        x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
        x ^= (x << 37) & 0xFFF7EEE000000000ULL;
        x ^= (x >> 43);

        return x;
    }

    class [[eosio::contract("spaceminer")]] space_contract : public eosio::contract{
    public:
        using contract::contract;

        space_contract( name s, name code, datastream<const char*> ds )
        :contract(s,code,ds)
        {}

	    [[eosio::action]]
        void setenv(uint32_t row, uint32_t column, const asset& click_price, const asset& grand_prize,
                    const asset& large_prize, const asset& medium_prize, const asset& small_prize, bool inGame,
                    uint64_t smallChance, uint64_t mediumChance, uint64_t largeChance, uint64_t grandChance);

        [[eosio::action]]
        void receiverand(uint64_t customer_id, const eosio::checksum256& random_value);

        [[eosio::action]]
        void transfer(uint64_t sender, uint64_t receiver);

        [[eosio::action]]
        void click(name account, uint32_t row, uint32_t column);

        [[eosio::action]]
        void restart();

    private:

        struct account {
            asset    balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        typedef eosio::multi_index<name("accounts"), account> accounts;

        // taken from eosio.token.hpp
        struct st_transfer {
            name  from;
            name  to;
            asset         quantity;
            std::string   memo;
        };


        struct [[eosio::table("global")]] eosio_global_state {
            uint64_t free_ram()const { return max_ram_size - total_ram_bytes_reserved; }

            uint64_t             max_ram_size = 64ll*1024 * 1024 * 1024;
            uint64_t             total_ram_bytes_reserved = 0;
            int64_t              total_ram_stake = 0;

            block_timestamp      last_producer_schedule_update;
            time_point           last_pervote_bucket_fill;
            int64_t              pervote_bucket = 0;
            int64_t              perblock_bucket = 0;
            uint32_t             total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
            int64_t              total_activated_stake = 0;
            time_point           thresh_activated_stake_time;
            uint16_t             last_producer_schedule_size = 0;
            double               total_producer_vote_weight = 0; /// the sum of all producer votes
            block_timestamp      last_name_close;

            // explicit serialization macro is not necessary, used here only to improve compilation time
            /*EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio::blockchain_parameters,
            (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)
                    (last_producer_schedule_update)(last_pervote_bucket_fill)
                    (pervote_bucket)(perblock_bucket)(total_unpaid_blocks)(total_activated_stake)(thresh_activated_stake_time)
                    (last_producer_schedule_size)(total_producer_vote_weight)(last_name_close) )*/
        };

        typedef eosio::singleton< "global"_n, eosio_global_state >   global_state_singleton;

        struct [[eosio::table("conf")]] space_env {
            uint32_t row;
            uint32_t column;
            asset click_price;
            uint32_t round;
            asset grand_prize;
            asset large_prize;
            asset medium_prize;
            asset small_prize;
            bool inGame;
            EOSLIB_SERIALIZE( space_env, (row)(column)(click_price)(round)
                    (grand_prize)(large_prize)(medium_prize)(small_prize)(inGame));
        };

        typedef eosio::singleton< "conf"_n, space_env > space_env_singleton;

        struct [[eosio::table("conf2")]] space_env_2 {
            uint64_t smallChance;
            uint64_t mediumChance;
            uint64_t largeChance;
            uint64_t grandChance;
            EOSLIB_SERIALIZE( space_env_2, (smallChance)(mediumChance)(largeChance)(grandChance));
        };

        typedef eosio::singleton< "conf2"_n, space_env_2 > space_env_singleton_2;

        struct [[eosio::table]] player{
            name account;
            uint32_t clicks;
            uint64_t primary_key() const { return account.value; }
            EOSLIB_SERIALIZE(player, (account)(clicks));
        };

        typedef eosio::multi_index<"players"_n, player> player_table;

        struct treasure{
            uint32_t x;
            uint32_t y;
            uint8_t type; // 0 for small, 1 for medium, 2 for large, 3 for grand
        };

        struct [[eosio::table("map")]] map_state{
            vector<uint8_t> map;
            EOSLIB_SERIALIZE(map_state, (map));
        };

        typedef eosio::singleton< "map"_n, map_state > map_singleton;
        
        uint64_t generateSign(uint64_t i);
    };
