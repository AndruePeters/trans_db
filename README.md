# trans_db
Transactional Database application

# trans_db
Transactional Database application

1. Automated C++ Engineering Test: Rollback to Consistency
Description

In this test you are tasked with creating a simple transactional database.  This database stores a simple integer balance for each account in its domain.  Accounts are identified by an integer.  This database supports transactions which can transfer some portion of the balance from one account to another.  Each transaction may have one or more transfers and transactions should be considered atomic: all the transfers of a transaction must either be applied or not-applied together.  Accounts may be overdrawn at any intermediate state of the database however, when the database is "settled" it must clean the database so that it has no overdrawn balances (they are all >= 0).  Any number of transactions can be dropped from any point in the input sequence to satisfy the invariant but they cannot be reordered and must remain atomic.  This means that for any two transactions (X, Y) that exist in both the input sequence of transactions and the output sequence of transactions, if X appears before Y in the input sequence it must appear before Y in the output sequence.



The expected result of your implementation is any valid listing of transactions and a final state of the accounts given the constraints above.  Solutions which tend to accept more of the provided input transactions are considered better but, the primary focus should be on correctness.


![example1_single_successful_transaction.png]
![example2_multiple_successful_transactions.png]
![example3_sing_failing_transaction.png]
![example4_multiple_xactions_that_restore_consistency.png]
