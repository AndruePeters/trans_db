#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <exception>
#include <memory>
using namespace std;

struct account_balance {
   int    account_id; ///< the name of the account
   int    balance; ///< the balance of the account
};

struct transfer {
   int from;    ///< the account to transfer from
   int to;      ///< the account to transfer to
   int amount;  ///< the amount to transfer
};

using transaction = vector<transfer>;
//typedef vector<transfer> transaction;
/**
 * @brief Stores the net change for each account in a transaction.
 *        transfer {1, 2, 5} equates to account_balance {1, -5} and account_balance {2, 5}.
 *
 *        Constructor throws std::invalid_argument if input is bad.
 * Reasons:
 *    Condenses a transaction into a state where the total number of entries is the  number of accounts used.
 *    Average case has less entries to process for rollback.
 *    Beneficial 
 */
class transaction_log {
public:
   transaction_log(const transaction& t, const size_t trans_id, const std::function<bool(const transfer& xfer)>& validate);
   void dump() const {
      for (const auto& x: log) {
         std::cout << "account: " << x.second.account_id << "\tbalance: " << x.second.balance << std::endl;
      }
   };

private:
   const std::function<bool(const transfer& xfer)>& validate_transfer;
   const size_t transaction_id;
   void build_log(const transaction& t);
   void add_to_log(const transfer& xfer);
   std::unordered_map<int, account_balance> log;
};

transaction_log::transaction_log(const transaction& t, const size_t trans_id,const std::function<bool(const transfer& xfer)>& validate): 
                                 transaction_id(trans_id), validate_transfer(validate)
{
   build_log(t);
}

void transaction_log::build_log(const transaction& t)
{
   for (const auto& xfer: t) {
      // if a single transfer is bad then drop the entire transaction because a transaction is atomic.
      if (!validate_transfer(xfer)) {
         throw std::invalid_argument("Account does not exist.");
      }

      // if this succeeds then add the valid transfer to the log
      add_to_log(xfer);
   }
}

void transaction_log::add_to_log(const transfer& xfer)
{
   // if xfer.from exists then add the balance, otherwise create it
   if (log.count(xfer.from) ) {
      log[xfer.from].balance -= xfer.amount;
   } else {
      // does not exist, so create
      log.insert({xfer.from, {xfer.from, 0-xfer.amount}});
   }

   // if xfer.to exists then add the balance, otherwise create it
   if (log.count(xfer.to)) {
      log[xfer.to].balance += xfer.amount;
   } else {
      // does not exist so create
      log.insert({xfer.to, {xfer.to, xfer.amount}});
   }
}



/**
 * @brief Transactional database implementation.
 *
 * All transactions must be atomic.
 * A "settle[d]" state cannot contain an account with a negative balance.
 */
class transaction_db {
public:
   using log_ptr = std::unique_ptr<transaction_log>;
   explicit transaction_db(const vector<account_balance>& initial_balances);
   void push_transaction(const transaction& t);
   void settle();
   vector<account_balance> get_balances() const;
   vector<size_t> get_applied_transactions() const;
   
private: 
   void push_transfer(const transfer& tfer);

private:
   size_t current_transaction;
   unordered_map<size_t , account_balance> accounts;
   vector<log_ptr> temp_log; ///< resets after every settle
   vector<size_t> applied_transactions;


   
};

transaction_db::transaction_db(const vector<account_balance>& initial_balances): 
               current_transaction(0), accounts(initial_balances.size())
{
   std::transform(initial_balances.begin(), initial_balances.end(),
                  std::inserter(accounts, accounts.end()),
                  [](const auto& accnt) { return std::make_pair(accnt.account_id, accnt); });
}

void transaction_db::push_transaction(const transaction& t)
{
   const size_t num = 0;
   log_ptr l;
   try {
      auto validate = [this](const transfer& xfer) {
         return accounts.count(xfer.to) && accounts.count(xfer.from);
      };

      l = std::make_unique<transaction_log>(transaction_log(t, num, validate));

   } catch (std::exception &e) {
      std::cout << "Bad data" << std::endl;
      return; // exit early
   }
   temp_log.push_back(std::move(l));
}

void transaction_db::settle()
{

}

vector<account_balance> transaction_db::get_balances() const
{
   vector<account_balance> balances;
   balances.reserve(accounts.size());

   std::transform(accounts.begin(), accounts.end(), 
                  std::back_inserter(balances), 
                  [] (const auto& pair) { return pair.second; });
  
   return balances;
}

vector<size_t> transaction_db::get_applied_transactions() const
{
   return applied_transactions;
}
/**
 *
 * @param initial_balances - the initial balances in the database, see the above datastructures.
 * @return an instantiated database type of your creation which has the following member methods:
 *
 *    // push a transaction to the database
 *    //
 *    void push_transaction( const transaction&t )
 *
 *    // settle the database such that the invariant is maintained and the best state of the database is present
 *    //
 *    void settle()
 *
 *    // return a listing of all the balances in any order
 *    //
 *    vector<account_balance> get_balances() const
 *
 *    // Given the logical sequence of transactions constructed by the ordered sequence of calls
 *    // to push_transaction AND having called settle()
 *    //
 *    // return the 0-based indices of the surviving transactions in that sequence which, when
 *    // applied to the initial balances produce a state where the invariant is maintained
 *    //
 *    vector<size_t> get_applied_transactions() const
 *
 * The automated test cases will call create_database with a set of initial balances
 * They will then feed a number of transactions to your class
 * They will then call settle
 * They will then call get_applied_transactions and inspect the results
 * They will then call get_balances and inspect the results
 */
auto create_database(const vector<account_balance>& initial_balances ) {
   return transaction_db(initial_balances);
}


template<typename DB>
static void print_transactions( const DB& db, ofstream& fout ) {
   auto trxs = db.get_applied_transactions();
   sort(trxs.begin(), trxs.end());
   fout << trxs.size() << endl;
   for( size_t cur : trxs ) {
      fout << cur << endl;
   }
}

template<typename DB>
static void print_database( const DB& db, ofstream& fout ) {
   auto balances = db.get_balances();
   sort(balances.begin(), balances.end(), [](const auto& a, const auto& b){
      return a.account_id < b.account_id;
   });

   fout << balances.size() << endl;
   for( const auto& cur : balances ) {
      fout << cur.account_id << " " << cur.balance << endl;
   }
}

int main() {

   try {

       ifstream fin;
       auto input_path = "input1.txt";//getenv("INPUT_PATH");
       if (input_path != nullptr) {
          fin.open(input_path);
       }

       istream& in =  input_path ? fin : cin;

       int remaining_accounts = 0;
       in >> remaining_accounts;
       in.ignore(numeric_limits<streamsize>::max(), '\n');

       vector<account_balance> initial_balances;
       while (remaining_accounts-- > 0) {
          int account = 0;
          int balance = 0;

          in >> account >> balance;
          in.ignore(numeric_limits<streamsize>::max(), '\n');

          initial_balances.push_back({account, balance});
       }

       auto db = create_database(initial_balances);

       int remaining_transactions = 0;
       in >> remaining_transactions;
       in.ignore(numeric_limits<streamsize>::max(), '\n');
       while (remaining_transactions-- > 0) {

          int remaining_transfers = 0;
          in >> remaining_transfers;
          in.ignore(numeric_limits<streamsize>::max(), '\n');

          transaction tx;

          while( remaining_transfers-- > 0) {
             int from = 0;
             int to = 0;
             int amount = 0;

             in >> from >> to >> amount;
             in.ignore(numeric_limits<streamsize>::max(), '\n');
             tx.push_back({from, to, amount});
          }

          db.push_transaction(tx);
       }

       db.settle();

      ofstream fout("out.txt");
       print_transactions(db, fout);

       print_database(db, fout);
   } catch (...) {
      return -1;
   }


   return 0;
}
