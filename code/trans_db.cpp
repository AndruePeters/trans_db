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


/**
 * @brief Stores the net change for each account in a transaction.
 *        If a transfer is invalid, then an std::invalid_argument error is thrown. 
 *        This exception is caught by transaction_db in push_transaction.
 * 
 *        transfer {1, 2, 5} equates to account_balance {1, -5} and account_balance {2, 5}.
 *         For larger transactions and number of accounts, this method reduces spacial needs and time required to iterate through.
 *
 *        Constructor throws std::invalid_argument if input is bad.
 *        Enforces the concept of being atomic by only allowing the constructor to build the log.
 *
 * Reasons:
 *    Condenses a transaction into a state where the total number of entries is the  number of accounts used.
 *       *Let there be M transfers with N unique accounts -> Only N entries will be stored.
 *       *Fasciliates faster settle[ing] because totals are already calculated per account; no need to iterate through all transfer to find total.
 *
 *    Average case has less entries to process for rollback. (Only cases for really small transactions might be faster, but those aren't practical or a true concern). 
 *    Used to verify a transaction before marking changes in the database.
 *    Beneficial spacially and temporally. 
 */
class transaction_log {
   using log_t = std::unordered_map<int, account_balance>;
public:
   using const_iterator = log_t::const_iterator;

   /**
    * @brief   Builds the transaction log. Reinforces idea that a transaction is atomic and can't change.
    * @param   t Transaction used to build the log.
    * @param   trans_id ID used to keep track of each transaction. Stored if transaction is used when the database is settled.
    * @param   validate Function used to verify that a transfer is valid.
    *
    * @throw   std::invalid_argument If a transfer is "invalid". Invalid currently means that the from and to account do not exist.
    */
   explicit transaction_log(const transaction& t, const size_t trans_id, const std::function<bool(const transfer& xfer)>& validate);

   /**
    * @return const_iterator to the beginning of the log.
    */
   const_iterator begin()  const { return log.cbegin(); }

   /**
    * @return const_iterator to the end of the log.
    */
   const_iterator end()    const { return log.cend(); }

   size_t get_transaction_id() const { return transaction_id; }

   /**
    * @return True if account_id is exists, false otherwise.
    */
   bool transfer_exists(const int account_id) const { return log.count(account_id); }

   /**
    * @return the net change for an account if it exists, 0 otherwise.
    */
   int net_change(const int account_id) const;

   /**
    * @brief Outputs all transfers within this transaction to stdout
    */
   void dump() const {
      for (const auto& x: log) {
         std::cout << "account: " << x.second.account_id << "\tbalance: " << x.second.balance << std::endl;
      }
   };

private:
   const size_t transaction_id; ///< stores the unique id given to the transaction
   const std::function<bool(const transfer& xfer)>& validate_transfer; ///< function to validate input
   log_t log;  ///< log used to store net account changes for transaction

   /**
    * @brief Builds the log.
    * @throw std::invalid_argument
    */
   void build_log(const transaction& t);

   /**
    * @brief Adds a transfer to the log.
    */
   void add_to_log(const transfer& xfer);
};

/**
 * @brief Transactional database implementation. Follows ACID properties.
 *
 * 
 * All transactions must be atomic.
 * A "settle[d]" state cannot contain an account with a negative balance.
 *
 * An unordered_map is used.
 *    * Order is not important.
 *    * Account id can be any type
 *    * Constant time look ups.
 * 
 * 
 * Variables:
 *    current_transaction keeps track of the most recent transaction
 *    accounts uses an unordered_map because order does not matter, account_id can become any type such as a string in future changes, constant time lookups
 *    temp_log is a map to preserve ordering, quick access time. chose a map over a vector because algorithm can commonly remove elements from "middle" of bounds
 *    applied_transactions is a set to enforce that there is a unique transaction id and will always remain ordered
 */
class transaction_db {
   using log_ptr = std::unique_ptr<transaction_log>;

public:

   /**
    * @brief Builds the initial database state from initial_balances.
    */
   explicit transaction_db(const vector<account_balance>& initial_balances);

   /**
    * @brief Pushes a transaction and loads it into the database. If a single transfer is invalid, then the entire transaction is drooped.
    */
   void push_transaction(const transaction& t);

   /**
    * @brief Commits changes and ensures the database is in a valid state.
    * Valid is defined as all accounts having a balance greater than 0.
    */
   void settle();

   /**
    * @return vector<account_balance> of current accounts.
    */
   vector<account_balance> get_balances() const;

   /**
    * @return vector<size_t> of all vectors that have been commited and used after a call to settle()
    */
   vector<size_t> get_applied_transactions() const;

   /**
    * @brief Rolls back transaction based on the transaction_log.
    */
    void rollback(const transaction_log& tlog);

   
private: 
   /**
    * @brief Updates database account balances after a transaction has been validated.
    */
   void apply_transaction(const transaction_log& tlog);


   /**
    * @return number of account_id's that have a negative balance from current database.
    */
   size_t get_invalid_accounts() const;

   /**
    * @return number of account_id's that have a negative balance AFTER a simulated application of t.
    *
    * @param t    A rollback of t is simulated.
    */
   size_t get_invalid_accounts(const transaction_log& t) const;

private:
   size_t current_transaction; ///< the current transaction
   unordered_map<size_t , account_balance> accounts;  ///< the database of accounts
   map<size_t, log_ptr> temp_log; ///< resets after every settle, size_t is the transaction number
   set<size_t> applied_transactions; ///< stores applied transactions and guarantees order
};


/**
 * Builds a transaction log and sets related varaibles.
 * Will not catch exception thrown from build_log. This is to be handled from wherever the transaction_log constructor is called.
 */
transaction_log::transaction_log(const transaction& t, const size_t trans_id,const std::function<bool(const transfer& xfer)>& validate): 
                                 transaction_id(trans_id), validate_transfer(validate)
{
   build_log(t);
}


/**
 * Iterates through all transfers in transaction and adds them to the log.
 * It aborts and throws std::invalid_arugment if a transfer is found to be invalid.
 */
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


/**
 * Subtracts xfer.balance from xfer.from and adds xfer.balance to xfer.to.
 * This function is ran after it is verified that the accounts in the transfer exist in the database.
 */
void transaction_log::add_to_log(const transfer& xfer)
{
   // if xfer.from exists then subtract the balance, otherwise create it
   if (log.count(xfer.from) ) {
      log[xfer.from].balance -= xfer.amount;
   } else {
      // does not exist, so create
      log.insert({xfer.from, {xfer.from, -xfer.amount}});
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
 * @return the net change for an account if it exists, 0 otherwise.
 */
int transaction_log::net_change(const int account_id) const
{
   auto map_it = log.find(account_id);
   if (map_it != log.end()) {
      return map_it->second.balance;
   } else {
      return 0;
   }
}





/**
 * Builds the database from a vector.
 * Uses std::transform to "transform" given vector to unordered_map.
 */
transaction_db::transaction_db(const vector<account_balance>& initial_balances): 
               current_transaction(0), accounts(initial_balances.size())
{
   // convert given initial account to the unordered_map form used internally
   // benefits from parallel execution in a C++17 compiler if execution policy is added to std::transform
   std::transform(initial_balances.begin(), initial_balances.end(),
                  std::inserter(accounts, accounts.end()),
                  [](const auto& accnt) { return std::make_pair(accnt.account_id, accnt); });
}

/**
 * Attemps to build a transaction log. If transaction_log throws then it returns early from the 
 * c'tor and is not applied to the database.

 * When transaction_log succeeds, t is applied to the database and the transaction log pushed into the temp_log.
 */
void transaction_db::push_transaction(const transaction& t)
{
   log_ptr xction_ptr; // only declared here for scope reasons
   try {
      auto validate = [this](const transfer& xfer) {
         return accounts.count(xfer.to) && accounts.count(xfer.from);
      };

      xction_ptr = std::make_unique<transaction_log>(transaction_log(t, current_transaction, validate));
   } catch (std::exception &e) {
      std::cerr << e.what();
      return; // exit early
   }
   apply_transaction(*xction_ptr);
   temp_log.emplace(xction_ptr->get_transaction_id(), std::move(xction_ptr));
   ++current_transaction; // increment the current_transaction
}

/**
 * Changes from the most recent transaction applied to database.
 */
void transaction_db::apply_transaction(const transaction_log& tlog)
{
   // pair.second is type account_balance
   for (const auto& accnt: tlog) {
      accounts[accnt.second.account_id].balance += accnt.second.balance;
   }
}


/**
 * @brief Puts database into a valid state by removing valid transactions.
 *
 * Algorithm Steps:
 * 1) Check if there are any negative account balances. If not, then commit changes and exit.
 * 2) Store the number of negative account balances in vector from simulating rolling back transactions in temp_log.
 * 3) Sort vector based on the number invalid account balances, with the fewest at the beginning.
 * 4) Rollback and delete the first transaction.
 * 5) Call settle() again. (Goto step 1)
 *
 * Main Assumption for Algorithm: Choosing results by fewest possible invalid accounts will lead to fewer transactions being rolled back.
 *
 * Thoughts:
 *    Performance could probably be improved by storing information when a transaction is applied at an intermediate step.
 *    However, the challenge here is finding an optimal solution without calculating every single combination.
 *          For example, assume there are three transactions {A, B, C}. 
 *             Calling get_invalid_accounts() changes for every unique intermediate state.
 *             In other words, get_invalid_accounts(A) != get_invalid_accounts(A) if B is rolled back.
 *             This is because get_invalid_accounts(transaction) returns the number of invalid accounts by
 *                simulating a rollback of that transaction.
 *
 *    Because of this, I thought it would be good if a local optimal was chosen until a solution is found.
 *    This approach does not guarantee that the global optimal solution is found.
 *       Example: Assume this algorithm uses four transactions {A, B, C, D}.
 *                Possible solution is to rollback in the following order {D, C, B}.
 *                This solution means that in the first pass, D had the least simulated invalid accounts, C in the second pass, and then B.
 *                However, the solution to rolling back the fewest transactions could be {A}, where A had the most invalid accounts in the first pass.
 *                Since this algorithm does not check future possibilities, the shortest solution was not found.
 *
 *    Could most likely reimplement this algorithm as a dynamic algorithm and find the optimal solution.
 *          To do this would mean changing my criteria from the number of invalid accounts to the number of transactions needed to rollback.
 *          Instead of get_invalid_accounts(), I'd define has_invalid_accounts(), which would return as soon as a single invalid account is found, thereby improving time complexity.
 *                Although, worst case would still be the same O(M) where M is the number of accounts in a transaction.
 *          This basically transforms this problem into a shortest path problem.
 *          Again, this biggest issue is that order of rollback matters and changes the state completely, so memoization might not be possible.
 *
 *    A brute force method would be possible to implement, but would scale very poorly as it would compute N! situtations where N is the number of transactions.
 *       Maybe I could look a few steps into the future to choose the best solution?
 *
 *    One approach I started to use looked at the specific accounts that were invalid; however, this fails because a transaction that fixes account 1 might make account 2 negative.
 *    Could easily make this algorithm iterative if I needed to. It would be in form while (get_invalid_accounts()) { // do stuff}
 */
void transaction_db::settle()
{
   // check if there are any invalid accounts in the current, intermediate database state
   // if there are none then save the transaction_id's, clear temp_log, and exit
   // 1)
   if (get_invalid_accounts() == 0) {
      for (const auto& x: temp_log) {
         applied_transactions.insert(x.second->get_transaction_id());
      }

      temp_log.clear();
      return;
   }

   // so we have invalid accounts, so now search for the transaction that results in the smallest number of invalid accounts
   // std::pair<vector.size(), index to transaction in temp_log>; idx is the current index
   // 2)
   std::vector<std::pair<size_t, size_t>> sia; ///< simulated invalid accounts
   std::for_each(temp_log.begin(), temp_log.end(), 
         [&sia, this] (const auto& tlog_ptr) mutable {
            sia.emplace_back(get_invalid_accounts(*(tlog_ptr.second)), tlog_ptr.second->get_transaction_id());
         });

   // sort so that the first element is the one we want to access     
   // 3) 
   std::sort(sia.begin(), sia.end());

   // now rollback the transaction that gives the smallest number of invalid balances.
   // xxx.second contains the index
   // 4)
   rollback(*temp_log[sia.front().second]);

   // now delete the rollbacked transaction
   temp_log.erase(temp_log.find(sia.front().second));

   // now do it all again
   // 5)
   settle();
}


/**
 * Transforms map into balance and returns the built vector.
 * **SHOULD** make used of NRVO. 
 */
vector<account_balance> transaction_db::get_balances() const
{
   vector<account_balance> balances;
   balances.reserve(accounts.size());

   std::transform(accounts.begin(), accounts.end(), 
                  std::back_inserter(balances), 
                  [] (const auto& pair) { return pair.second; });
  
   return balances;
}

/**
 * Returns a copy of applied_transactions.
 * Firstly copies applied_transactions and should be valid for RVO.
 */
vector<size_t> transaction_db::get_applied_transactions() const
{
   return std::vector<size_t>(applied_transactions.begin(), applied_transactions.end());
}

/**
 * @brief Rolls back transaction based on the transaction_log.
 *
 * All values from tlog will exist in database.
 */
void transaction_db::rollback(const transaction_log& tlog)
{
   for (const auto& t: tlog) {
      accounts[t.second.account_id].balance -= t.second.balance; 
   }
}

/**
 * @return number of account_id's that have a negative balance from current database.
 */
size_t transaction_db::get_invalid_accounts() const
{
   return  std::count_if(accounts.begin(), accounts.end(), 
         [](const auto& xfer){
            return xfer.second.balance < 0;
         });
}

/**
 * @return number of account_id's that have a negative balance AFTER a simulated application of t.
 *
 * @param t    A rollback of t is simulated.
 */
size_t transaction_db::get_invalid_accounts(const transaction_log& t) const
{
   size_t invalid_accounts = 0;
   
   for (const auto& x: t) {
      auto i = accounts.find(x.second.account_id);
      int new_difference = i->second.balance - t.net_change(x.second.balance);
      if (new_difference < 0) {
        ++invalid_accounts;
      }
   }
   return invalid_accounts;
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
