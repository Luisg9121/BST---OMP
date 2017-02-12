/*
Binary search tree code for CS 4380 / CS 5351

Copyright (c) 2016, Texas State University. All rights reserved.

Redistribution in source or binary form, with or without modification,
is not permitted. Use in source and binary forms, with or without
modification, is only permitted for academic use in CS 4380 or CS 5351
at Texas State University.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Author: Martin Burtscher & Luis Gomez(lg1336)
*/

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <sys/time.h>
#include <omp.h>

static int count = 0;

struct BSTnode {
  unsigned int val;
  BSTnode* left;
  BSTnode* right;
  omp_lock_t lock;
};

static unsigned int hash(unsigned int val)
{
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = ((val >> 16) ^ val);

  return val;
}

static void insert(BSTnode* &root, const unsigned int val){ 
  if (root == NULL) {
    root = new BSTnode;
    //initializing lock where new node is going to be
    omp_init_lock(&root->lock);
    root->val = val;
    root->left = NULL;
    root->right = NULL;
    count++; 
    }
    //finding parent node
   else if(val <= root->val && root->left == NULL){
      //1st lock set
      omp_set_lock(&root->lock);
      //if left still points to null(hasnt been changed), insert node
      if(root->left == NULL){
        insert(root->left, val);
        omp_unset_lock(&root->lock);
        }
      //another thread has written to position
      else{ 
         omp_unset_lock(&root->lock);
         insert(root->left,val);
         }
      }
      //keep traversing BST to find insert location
    else if(val <= root->val){
         insert(root->left, val);
       }
     //finding parent node 
    else if(root->right == NULL){
      //set 1st lock
       omp_set_lock(&root->lock);
      //if position hasnt been written to, insert node
       if(root->right == NULL){
          insert(root->right, val);
          omp_unset_lock(&root->lock);
          }
      //position has already been written to by another thread
       else{
           omp_unset_lock(&root->lock);
           insert(root->right, val);
           }  
        }
     //traverse BST to find insertion place
    else
      insert(root->right, val);
     
}

static BSTnode* buildBST(const int n, const int seed, int num_threads){
  BSTnode* root = NULL;
 
  insert(root, hash(seed)); 

  unsigned int i;

  #pragma omp parallel for num_threads(num_threads) default(none) shared(root) private(i)
  for (i = 1; i < n; i++) {
    insert(root, hash(i ^ seed));
    }
  return root;
}

static int verifyAndDeallocate(const BSTnode* const r){

  static int total_count = 0;//variable to hold number of nodes deleted

  if (r->left != NULL) {
    if (r->left->val > r->val){
       fprintf(stderr, "error: left subtree contains larger value\n"); 
       exit(-1);
       } 
       verifyAndDeallocate(r->left);
     }
  if (r->right != NULL){
    if (r->right->val <= r->val){
       fprintf(stderr, "error: right subtree contains smaller or equal value\n"); 
       exit(-1);
       }
       verifyAndDeallocate(r->right);
     } 
  //increment count of number nodes that have been deleted
  total_count++; 

  delete r;

  return total_count; 
}

int main(int argc, char* argv[])
{
  printf("\n\nBST v1.0 [OpenMP]\n");

  // check command line
  if (argc != 4){
     fprintf(stderr, "usage: %s number_of_values random_seed\n", argv[0]); 
     exit(-1);
     }

  // read command line parameters
  int n = atoi(argv[1]); 
  if (n < 1){
     fprintf(stderr, "error: number of values must be at least 1\n"); 
     exit(-1);
     }
  int seed = atoi(argv[2]);

  printf("configuration: %d values with seed of %d\n", n, seed);

  int num_threads = atoi(argv[3]);

  if(num_threads < 1){
     printf("Number of threads must be at least 1"); 
     exit(-1);
     }

  printf("Requested number of threads to run: %d\n", num_threads);

  // time BST creation
  struct timeval start, end;
  gettimeofday(&start, NULL);
  BSTnode* root = buildBST(n, seed, num_threads);
  gettimeofday(&end, NULL);

  double runtime = end.tv_sec + end.tv_usec / 1000000.0 - start.tv_sec - start.tv_usec / 1000000.0;
  printf("compute time: %.4f s\n", runtime);
  printf("throughput: %.3f Mvalues/s\n", n * 0.000001 / runtime);

  // verify result
  if (root == NULL){
     fprintf(stderr, "error: no tree was built\n"); 
     exit(-1);}

  int local_count =  verifyAndDeallocate(root);
  
  //verify all nodes have been deleted
  if(local_count != n)
    {
    printf("Number of nodes deleted does not match tree size\n");
    }

  //print number of nodes removed
  printf("Node removal count: %d\n\n", local_count);

  return 0;
}

