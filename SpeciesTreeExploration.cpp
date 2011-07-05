
/* This file contains various functions useful for the search of the species tree*/

#include "SpeciesTreeExploration.h"




/************************************************************************
 * Procedure that makes NNIs and ReRootings by calling makeDeterministicNNIsAndRootChangesOnly.
************************************************************************/


void localOptimizationWithNNIsAndReRootings(const mpi::communicator& world, 
                                            TreeTemplate<Node> *tree, 
                                            TreeTemplate<Node> *bestTree, 
                                            int &index, 
                                            int &bestIndex,  
                                            bool &stop, 
                                            int timeLimit,
                                            double &logL, 
                                            double &bestlogL, 
                                            std::vector<int> &num0Lineages, 
                                            std::vector<int> &num1Lineages,
                                            std::vector<int> &num2Lineages, 
                                            std::vector<int> &bestNum0Lineages, 
                                            std::vector<int> &bestNum1Lineages,
                                            std::vector<int> &bestNum2Lineages, 
                                            std::vector< std::vector<int> > &allNum0Lineages, 
                                            std::vector< std::vector<int> > &allNum1Lineages,
                                            std::vector< std::vector<int> > &allNum2Lineages, 
                                            std::vector<double> &lossExpectedNumbers, 
                                            std::vector<double> &duplicationExpectedNumbers, 
                                            /*double averageDuplicationProbability, 
                                            double averageLossProbability,*/ 
                                            bool rearrange, 
                                            int &numIterationsWithoutImprovement, 
                                            int server, 
                                            int & nodeForNNI, 
                                            int & nodeForRooting, 
                                            std::string & branchProbaOptimization, 
                                            std::map < std::string, int> genomeMissing,
                                            std::vector < double >  &NNILks, 
                                            std::vector<double> &rootLks) 
{
  
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  std::string currentSpeciesTree;
  while (!stop) 
    {
    //If we have already computed the value for the NNI or the rerooting 
    //we are about to make, we don't make it!

    stop = checkChangeHasNotBeenDone(*tree, bestTree, nodeForNNI, nodeForRooting, NNILks, rootLks);


    if (!stop) 
      {
      makeDeterministicNNIsAndRootChangesOnly(*tree, nodeForNNI, nodeForRooting);  

      computeSpeciesTreeLikelihoodWhileOptimizingDuplicationAndLossRates(world, index, stop, 
                                                                         logL, num0Lineages, 
                                                                         num1Lineages,num2Lineages, 
                                                                         allNum0Lineages, allNum1Lineages, 
                                                                         allNum2Lineages, lossExpectedNumbers, 
                                                                         duplicationExpectedNumbers, rearrange, 
                                                                         server, branchProbaOptimization, 
                                                                         genomeMissing, *tree, bestlogL);
      
      if ((nodeForNNI <3) /*&& (nodeForRooting == 4)*/) //A NNI has been done
        {
        if (logL < rootLks[nodeForRooting-1]) 
          {
          rootLks[nodeForRooting-1] = logL;
          }
        }
      else {
        if (logL < NNILks[nodeForNNI-1]) 
          {
          NNILks[nodeForNNI-1] = logL;
          }
      }
        //int branchId = bestTree->getNode(nodeForNNI-1)->getFather()->getId();
      /*  if (logL < NNILks[nodeForNNI]) 
          {
          NNILks[nodeForNNI] = logL;
          }
        }
      else //A rerooting has been done 
        {
        if (logL < rootLks[nodeForRooting]) 
          {
          rootLks[nodeForRooting] = logL;
          }
        }*/
      
      
      if (logL+0.01<bestlogL) 
        {
        std::cout << "\t\tNNIs or Root changes: Improvement: new total Likelihood value "<<logL<<" compared to the best log Likelihood : "<<bestlogL<< std::endl;
        std::cout << "Improved species tree: "<<TreeTools::treeToParenthesis(*tree, true)<< std::endl;
        numIterationsWithoutImprovement = 0;
        bestlogL =logL;
        if (bestTree)
          {
          delete bestTree;
          }
        bestTree = tree->clone();  
        bestDupProba=duplicationExpectedNumbers;
        bestLossProba=lossExpectedNumbers;
        bestNum0Lineages = num0Lineages;
        bestNum1Lineages = num1Lineages;
        bestNum2Lineages = num2Lineages;
        bestIndex = index;
        for (int i = 0 ; i< NNILks.size() ; i++ ) 
          {
          NNILks[i]=NumConstants::VERY_BIG;
          rootLks[i]=NumConstants::VERY_BIG;
          }
        if (ApplicationTools::getTime() >= timeLimit)
          {
          stop = true;
          broadcast(world, stop, server); 
          broadcast(world, bestIndex, server);
          }
        }
      else 
        {
        numIterationsWithoutImprovement++;
        std::cout <<"\t\tNNIs or Root changes: Number of iterations without improvement: "<<numIterationsWithoutImprovement<< std::endl;
        delete tree;
        tree = bestTree->clone();
        duplicationExpectedNumbers = bestDupProba;
        lossExpectedNumbers = bestLossProba;
        if ( (numIterationsWithoutImprovement>2*tree->getNumberOfNodes()) || (ApplicationTools::getTime() >= timeLimit) ) 
          {          
            stop = true; 
            broadcast(world, stop, server); 
            broadcast(world, bestIndex, server);
          }
        }
      }
    else //stop is true
      {  
        stop = false;
        computeSpeciesTreeLikelihood(world, index, stop, 
                                     logL, num0Lineages, 
                                     num1Lineages,num2Lineages, 
                                     allNum0Lineages, allNum1Lineages, 
                                     allNum2Lineages, lossExpectedNumbers, 
                                     duplicationExpectedNumbers, rearrange, 
                                     server, branchProbaOptimization, 
                                     genomeMissing, *tree);
        stop = true;
        broadcast(world, stop, server); 
        broadcast(world, bestIndex, server);
      }
    }
}







/************************************************************************
 * Only optimizes duplication and loss rates
************************************************************************/
void optimizeOnlyDuplicationAndLossRates(const mpi::communicator& world, 
                                         TreeTemplate<Node> *tree, 
                                         TreeTemplate<Node> *bestTree, 
                                         int &index, 
                                         int &bestIndex,  
                                         bool &stop,
                                         int timeLimit,
                                         double &logL, 
                                         double &bestlogL, 
                                         std::vector<int> &num0Lineages, 
                                         std::vector<int> &num1Lineages, 
                                         std::vector<int> &num2Lineages, 
                                         std::vector<int> &bestNum0Lineages, 
                                         std::vector<int> &bestNum1Lineages, 
                                         std::vector<int> &bestNum2Lineages, 
                                         std::vector< std::vector<int> > &allNum0Lineages, 
                                         std::vector< std::vector<int> > &allNum1Lineages, 
                                         std::vector< std::vector<int> > &allNum2Lineages, 
                                         std::vector<double> &lossExpectedNumbers, 
                                         std::vector<double> &duplicationExpectedNumbers, 
                                         /*double averageDuplicationProbability, 
                                         double averageLossProbability,*/ 
                                         bool rearrange, 
                                         int &numIterationsWithoutImprovement, 
                                         int server, 
                                         int & nodeForNNI, 
                                         int & nodeForRooting, 
                                         std::string & branchProbaOptimization,
                                         std::map < std::string, 
                                         int> genomeMissing) {
  computeSpeciesTreeLikelihoodWhileOptimizingDuplicationAndLossRates(world, index, stop, 
                                                                     logL, num0Lineages, 
                                                                     num1Lineages,num2Lineages, 
                                                                     allNum0Lineages, allNum1Lineages, 
                                                                     allNum2Lineages, lossExpectedNumbers, 
                                                                     duplicationExpectedNumbers, rearrange, 
                                                                     server, branchProbaOptimization, 
                                                                     genomeMissing, *tree, bestlogL);
  bestIndex = index;
  stop = true;
  bestTree = tree;
  bestNum0Lineages = num0Lineages;
  bestNum1Lineages = num1Lineages;
  bestNum2Lineages = num2Lineages;
  broadcast(world, stop, server); 
  broadcast(world, bestIndex, server);
}



/************************************************************************
 * Tries all reRootings of the species tree, and executes the one with the highest likelihood. 
 Here we do not use branchwise rates of duplications and losses, 
 and we do not optimise these rates often. 
 However, there are lots of useless identical std::vector copies...
 ************************************************************************/
void fastTryAllPossibleReRootingsAndMakeBestOne(const mpi::communicator& world, 
                                                TreeTemplate<Node> *currentTree, 
                                                TreeTemplate<Node> *bestTree, 
                                                int &index, int &bestIndex,  
                                                bool stop, int timeLimit,
                                                double &logL, double &bestlogL, 
                                                std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, 
                                                std::vector<int> &num2Lineages, 
                                                std::vector<int> &bestNum0Lineages, 
                                                std::vector<int> &bestNum1Lineages, 
                                                std::vector<int> &bestNum2Lineages, 
                                                std::vector< std::vector<int> > &allNum0Lineages, 
                                                std::vector< std::vector<int> > &allNum1Lineages, 
                                                std::vector< std::vector<int> > &allNum2Lineages, 
                                                std::vector<double> &lossExpectedNumbers, 
                                                std::vector<double> &duplicationExpectedNumbers, 
                                                /*double averageDuplicationProbability, double averageLossProbability,*/ 
                                                bool rearrange, int &numIterationsWithoutImprovement, 
                                                int server, std::string &branchProbaOptimization, 
                                                std::map < std::string, int> genomeMissing, 
                                                bool optimizeRates) {  
  breadthFirstreNumber (*currentTree, duplicationExpectedNumbers, lossExpectedNumbers);
  std::vector <double> backupDupProba=duplicationExpectedNumbers;
  std::vector<double> backupLossProba=lossExpectedNumbers;
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  TreeTemplate<Node> *tree;
  tree = currentTree->clone();
  bool betterTree = false;
  std::vector <int> nodeIds = tree->getNodesId();
  for (int i =0 ; i<nodeIds.size() ; i++) {
    if ((nodeIds[i]!=0)&&(nodeIds[i]!=1)&&(nodeIds[i]!=2)) { //We do not want to try the root we're already at
      if (i!=0) {
        deleteTreeProperties(*tree);
        delete tree;
        tree = currentTree->clone();
      }
      changeRoot(*tree, nodeIds[i]);
      if (optimizeRates) 
        {
          computeSpeciesTreeLikelihoodWhileOptimizingDuplicationAndLossRates(world, index, stop, logL, /*lossNumbers, duplicationNumbers, branchNumbers, AllLosses, AllDuplications, AllBranches, */num0Lineages, num1Lineages,num2Lineages, allNum0Lineages, allNum1Lineages, allNum2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, rearrange, server, branchProbaOptimization, genomeMissing, *tree, bestlogL);
        }
      else 
        {
          computeSpeciesTreeLikelihood(world, index, stop, logL, /*lossNumbers, duplicationNumbers, branchNumbers, AllLosses, AllDuplications, AllBranches, */num0Lineages, num1Lineages,num2Lineages, allNum0Lineages, allNum1Lineages, allNum2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, rearrange, server, branchProbaOptimization, genomeMissing, *tree);
      }
      if (logL+0.01<bestlogL) { 
        numIterationsWithoutImprovement = 0;
        betterTree = true;
        bestlogL =logL;
        if (bestTree) {
          delete bestTree;
        }
        bestTree = tree->clone();   
        bestDupProba=duplicationExpectedNumbers;
        bestLossProba=lossExpectedNumbers;
        bestNum0Lineages = num0Lineages;
        bestNum1Lineages = num1Lineages;
        bestNum2Lineages = num2Lineages;
        bestIndex = index;
       std::cout <<"ReRooting: Improvement! : "<<numIterationsWithoutImprovement<< " logLk: "<<logL<< std::endl;
       std::cout << "Better candidate tree likelihood : "<<bestlogL<< std::endl; 
       std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
      }
      else {
        numIterationsWithoutImprovement++;
        std::cout <<"ReRooting: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< " logLk: "<<logL<< std::endl;
      }
    }
    if (ApplicationTools::getTime() >= timeLimit)
      {
      stop = true;
      broadcast(world, stop, server); 
      broadcast(world, bestIndex, server);      
      break;
      }
  }
  
  if (betterTree) {
    logL = bestlogL;  
    duplicationExpectedNumbers = bestDupProba;
    lossExpectedNumbers = bestLossProba;
    num0Lineages = bestNum0Lineages;
    num1Lineages = bestNum1Lineages;
    num2Lineages = bestNum2Lineages;
    delete currentTree;
    currentTree = bestTree->clone(); 
   std::cout << "\t\tServer: tryAllPossibleReRootingsAndMakeBestOne: new total Likelihood value "<<logL<< std::endl;
   std::cout << TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, true);
    breadthFirstreNumber (*currentTree, duplicationExpectedNumbers, lossExpectedNumbers);
    if (ApplicationTools::getTime() < timeLimit)
      {
      broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
      //COMPUTATION IN CLIENTS
      index++;  
      bestIndex = index;
      std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;    
      logL = 0.0;
      std::vector<double> logLs;
      resetVector(num0Lineages);
      resetVector(num1Lineages);
      resetVector(num2Lineages);
      gather(world, logL, logLs, server);
      logL =  VectorTools::sum(logLs);
      gather(world, num0Lineages, allNum0Lineages, server);
      gather(world, num1Lineages, allNum1Lineages, server);
      gather(world, num2Lineages, allNum2Lineages, server);
      }
  }
  else {
   std::cout<< "No improvement in fastTryAllPossibleReRootingsAndMakeBestOne"<< std::endl;
    logL = bestlogL;  
    duplicationExpectedNumbers = bestDupProba;
    lossExpectedNumbers = bestLossProba;
    num0Lineages = bestNum0Lineages;
    num1Lineages = bestNum1Lineages;
    num2Lineages = bestNum2Lineages;
    delete currentTree;
    currentTree = bestTree->clone(); 
  }
  deleteTreeProperties(*tree);
  delete tree;
}


/************************************************************************
 * Tries all SPRs at a distance dist for all possible subtrees of the subtree starting in node nodeForSPR, 
 and executes the ones with the highest likelihood. 
 Uses only average rates of duplication and loss, not branchwise rates.
 ************************************************************************/
void fastTryAllPossibleSPRs(const mpi::communicator& world, TreeTemplate<Node> *currentTree, 
                            TreeTemplate<Node> *bestTree, int &index, int &bestIndex,  
                            bool stop, int timeLimit, double &logL, double &bestlogL, 
                            std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, std::vector<int> &num2Lineages, 
                            std::vector<int> &bestNum0Lineages, std::vector<int> &bestNum1Lineages, std::vector<int> &bestNum2Lineages, 
                            std::vector< std::vector<int> > &allNum0Lineages, std::vector< std::vector<int> > &allNum1Lineages, 
                            std::vector< std::vector<int> > &allNum2Lineages, 
                            std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, 
                            /*double averageDuplicationProbability, double averageLossProbability,*/ 
                            bool rearrange, int &numIterationsWithoutImprovement, int server, 
                            std::string &branchProbaOptimization, std::map < std::string, int> genomeMissing, 
                            int sprLimit, bool optimizeRates) {
 std::cout <<"in fastTryAllPossibleSPRs : currentTree : "<< std::endl;
 std::cout<< TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;
  breadthFirstreNumber (*currentTree, duplicationExpectedNumbers, lossExpectedNumbers);
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  std::vector <int> nodeIdsToRegraft;
  bool betterTree;
  TreeTemplate<Node> *tree;
  for (int nodeForSPR=currentTree->getNumberOfNodes()-1 ; nodeForSPR >0; nodeForSPR--) {
    buildVectorOfRegraftingNodesLimitedDistance(*tree, nodeForSPR, sprLimit, nodeIdsToRegraft);
    betterTree = false;
    for (int i =0 ; i<nodeIdsToRegraft.size() ; i++) {
      if (tree) {
        delete tree;
      }
      tree = currentTree->clone();
      
      makeSPR(*tree, nodeForSPR, nodeIdsToRegraft[i]);
      if (optimizeRates) 
        {
          computeSpeciesTreeLikelihoodWhileOptimizingDuplicationAndLossRates(world, index, 
                                                                             stop, logL, 
        /*lossNumbers, duplicationNumbers, branchNumbers, AllLosses, AllDuplications, AllBranches, */
                                                                             num0Lineages, num1Lineages,num2Lineages, 
                                                                             allNum0Lineages, allNum1Lineages, allNum2Lineages, 
                                                                             lossExpectedNumbers, duplicationExpectedNumbers, 
                                                                             rearrange, server, branchProbaOptimization, 
                                                                             genomeMissing, *tree, bestlogL);
        }
      else 
        {
          computeSpeciesTreeLikelihood(world, index, 
                                       stop, logL, 
        /*lossNumbers, duplicationNumbers, branchNumbers, AllLosses, AllDuplications, AllBranches, */
                                       num0Lineages, num1Lineages,num2Lineages, 
                                       allNum0Lineages, allNum1Lineages, allNum2Lineages, 
                                       lossExpectedNumbers, duplicationExpectedNumbers, 
                                       rearrange, server, branchProbaOptimization, 
                                       genomeMissing, *tree);
        }
      if (logL+0.01<bestlogL) {
        betterTree = true;
        bestlogL =logL;
        deleteTreeProperties(*bestTree);
        delete bestTree;
        bestTree = tree->clone();  
        bestDupProba=duplicationExpectedNumbers;
        bestLossProba=lossExpectedNumbers;
        bestNum0Lineages = num0Lineages;
        bestNum1Lineages = num1Lineages;
        bestNum2Lineages = num2Lineages;
        bestIndex = index;
       std::cout << "SPRs: Better candidate tree likelihood : "<<bestlogL<< std::endl;
       std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
      }
      if (ApplicationTools::getTime() >= timeLimit)
        {
        stop = true;
        broadcast(world, stop, server); 
        broadcast(world, bestIndex, server);      
        break;
        }
    }
    if (betterTree) {
      logL = bestlogL; 
      duplicationExpectedNumbers = bestDupProba;
      lossExpectedNumbers = bestLossProba;
      numIterationsWithoutImprovement = 0;
      num0Lineages = bestNum0Lineages;
      num1Lineages = bestNum1Lineages;
      num2Lineages = bestNum2Lineages;
      deleteTreeProperties(*currentTree);
      delete currentTree;
      currentTree = bestTree->clone(); 
      breadthFirstreNumber (*currentTree);
     std::cout <<"SPRs: Improvement! : "<<numIterationsWithoutImprovement<< std::endl;
     std::cout << "\t\tServer: SPRs: new total Likelihood value "<<logL<< std::endl;
    
      if (ApplicationTools::getTime() < timeLimit)
        {
        //Send the new std::vectors to compute the new likelihood of the best tree
        std::string currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, true);
        broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
        //COMPUTATION IN CLIENTS
        index++;  
        bestIndex = index;
        std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
        logL = 0.0;
        std::vector<double> logLs;
        resetVector(num0Lineages);
        resetVector(num1Lineages);
        resetVector(num2Lineages);
        gather(world, logL, logLs, server);
        logL =  VectorTools::sum(logLs);
        gather(world, num0Lineages, allNum0Lineages, server);
        gather(world, num1Lineages, allNum1Lineages, server);
        gather(world, num2Lineages, allNum2Lineages, server);
        }
      else 
        {
        stop = true;
        broadcast(world, stop, server); 
        broadcast(world, bestIndex, server);      
        break;              
        }
    }
    else {
      logL = bestlogL;  
      duplicationExpectedNumbers = bestDupProba;
      lossExpectedNumbers = bestLossProba;
      num0Lineages = bestNum0Lineages;
      num1Lineages = bestNum1Lineages;
      num2Lineages = bestNum2Lineages;
      delete currentTree;
      currentTree = bestTree->clone(); 
      numIterationsWithoutImprovement++;
     std::cout <<"SPRs: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< std::endl;
    }
    deleteTreeProperties(*tree);
    delete tree;
    if (ApplicationTools::getTime() >= timeLimit)
      {
      stop = true;
      broadcast(world, stop, server); 
      broadcast(world, bestIndex, server);      
      break;      
      }
  }
}
 


/************************************************************************
 * Tries all SPRs and all rerootings with average rates of duplication and loss, 
 * not branchwise rates. Only does SPRs at a given distance. 
 ************************************************************************/
void fastTryAllPossibleSPRsAndReRootings(const mpi::communicator& world, 
                                         TreeTemplate<Node> *currentTree, 
                                         TreeTemplate<Node> *bestTree, 
                                         int &index, int &bestIndex,  
                                         bool stop, 
                                         int timeLimit,
                                         double &logL, 
                                         double &bestlogL, 
                                         std::vector<int> &num0Lineages, 
                                         std::vector<int> &num1Lineages, 
                                         std::vector<int> &num2Lineages, 
                                         std::vector<int> &bestNum0Lineages, 
                                         std::vector<int> &bestNum1Lineages, 
                                         std::vector<int> &bestNum2Lineages, 
                                         std::vector< std::vector<int> > &allNum0Lineages, 
                                         std::vector< std::vector<int> > &allNum1Lineages, 
                                         std::vector< std::vector<int> > &allNum2Lineages, 
                                         std::vector<double> &lossExpectedNumbers, 
                                         std::vector<double> &duplicationExpectedNumbers, 
                                         /*double averageDuplicationProbability, 
                                         double averageLossProbability,*/ 
                                         bool rearrange, 
                                         int &numIterationsWithoutImprovement, 
                                         int server, 
                                         std::string &branchProbaOptimization, 
                                         std::map < std::string, int> genomeMissing, 
                                         int sprLimit, 
                                         bool optimizeRates) {
  if (optimizeRates)
    {
    std::cout <<"Making SPRs and NNIs and optimizing duplication and loss rates."<< std::endl;
    }
  else 
    {
    std::cout <<"Making SPRs and NNIs but NOT optimizing duplication and loss rates."<< std::endl;
    }
  numIterationsWithoutImprovement=0;
  
  while (numIterationsWithoutImprovement<2*currentTree->getNumberOfNodes()) {
    fastTryAllPossibleSPRs(world, currentTree, bestTree, 
                           index, bestIndex,  
                           stop, timeLimit, 
                           logL, bestlogL, 
                           num0Lineages, num1Lineages, num2Lineages, 
                           bestNum0Lineages, bestNum1Lineages, bestNum2Lineages, 
                           allNum0Lineages, allNum1Lineages, allNum2Lineages, 
                           lossExpectedNumbers, duplicationExpectedNumbers, 
                           /*averageDuplicationProbability, averageLossProbability,*/ 
                           rearrange, numIterationsWithoutImprovement, 
                           server, branchProbaOptimization, genomeMissing, 
                           sprLimit, optimizeRates);

    if (ApplicationTools::getTime() >= timeLimit) 
      {	
        stop = true;
        broadcast(world, stop, server); 
        broadcast(world, bestIndex, server);      
        break;      
      }    
    if  (numIterationsWithoutImprovement>2*currentTree->getNumberOfNodes()) 
      {	
        break;
      }
    
    fastTryAllPossibleReRootingsAndMakeBestOne(world, currentTree, bestTree, 
                                               index, bestIndex,  
                                               stop, timeLimit, 
                                               logL, bestlogL, 
                                               num0Lineages, num1Lineages, num2Lineages, 
                                               bestNum0Lineages, bestNum1Lineages, bestNum2Lineages, 
                                               allNum0Lineages, allNum1Lineages, allNum2Lineages, 
                                               lossExpectedNumbers, duplicationExpectedNumbers, 
                                               /*averageDuplicationProbability, averageLossProbability,*/ 
                                               rearrange, numIterationsWithoutImprovement, 
                                               server, branchProbaOptimization, 
                                               genomeMissing, optimizeRates);
    std::cout << "after fastTryAllPossibleReRootingsAndMakeBestOne :currentTree: "<< std::endl;
    std::cout << TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;
    if (ApplicationTools::getTime() >= timeLimit) 
      {	
        stop = true;
        broadcast(world, stop, server); 
        broadcast(world, bestIndex, server);      
        break;      
      }    
    
  }
  
}




/************************************************************************
 * Broadcasts all necessary information. 
 ************************************************************************/
void broadcastsAllInformation(const mpi::communicator& world, int server, bool stop, bool &rearrange, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, std::string & currentSpeciesTree) {
  broadcast(world, stop, server);
  broadcastsAllInformationButStop(world,server, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
}
void broadcastsAllInformationButStop(const mpi::communicator& world, int server, bool &rearrange, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, std::string & currentSpeciesTree) {
  broadcast(world, rearrange, server); 
  broadcast(world, lossExpectedNumbers, server);
  broadcast(world, duplicationExpectedNumbers, server); 
  broadcast(world, currentSpeciesTree, server);
}

/************************************************************************
 * Computes the likelihood of a species tree, only once. 
 ************************************************************************/
std::string computeSpeciesTreeLikelihood(const mpi::communicator& world, 
                                         int &index, 
                                         bool stop, 
                                         double &logL, 
                                         std::vector<int> &num0Lineages, 
                                         std::vector<int> &num1Lineages, 
                                         std::vector<int> &num2Lineages, 
                                         std::vector< std::vector<int> > &allNum0Lineages, 
                                         std::vector< std::vector<int> > &allNum1Lineages, 
                                         std::vector< std::vector<int> > &allNum2Lineages, 
                                         std::vector<double> &lossExpectedNumbers, 
                                         std::vector<double> &duplicationExpectedNumbers, 
                                         bool rearrange, 
                                         int server, 
                                         std::string &branchProbaOptimization, 
                                         std::map < std::string, int> genomeMissing, 
                                         TreeTemplate<Node> &tree) 
{
  breadthFirstreNumber (tree, duplicationExpectedNumbers, lossExpectedNumbers);
  std::string currentSpeciesTree = TreeTools::treeToParenthesis(tree, true);
  computeSpeciesTreeLikelihoodWithGivenStringSpeciesTree(world,index, 
                                                         stop, logL, 
                                                         num0Lineages, num1Lineages, 
                                                         num2Lineages, allNum0Lineages, 
                                                         allNum1Lineages, allNum2Lineages, 
                                                         lossExpectedNumbers, duplicationExpectedNumbers, 
                                                         rearrange, server, 
                                                         branchProbaOptimization, genomeMissing, 
                                                         tree, currentSpeciesTree, false);
  return currentSpeciesTree;
}





/************************************************************************
 * Computes the likelihood of a species tree, only once. 
 ************************************************************************/
std::string computeSpeciesTreeLikelihoodWithGivenStringSpeciesTree(const mpi::communicator& world, 
                                         int &index, 
                                         bool stop, 
                                         double &logL, 
                                         std::vector<int> &num0Lineages, 
                                         std::vector<int> &num1Lineages, 
                                         std::vector<int> &num2Lineages, 
                                         std::vector< std::vector<int> > &allNum0Lineages, 
                                         std::vector< std::vector<int> > &allNum1Lineages, 
                                         std::vector< std::vector<int> > &allNum2Lineages, 
                                         std::vector<double> &lossExpectedNumbers, 
                                         std::vector<double> &duplicationExpectedNumbers, 
                                         bool rearrange, 
                                         int server, 
                                         std::string &branchProbaOptimization, 
                                         std::map < std::string, int> genomeMissing, 
                                         TreeTemplate<Node> &tree, 
                                         std::string currentSpeciesTree,
                                         bool firstTime) 
{
  if (!firstTime) 
    {
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    index++;  
    }

  //COMPUTATION IN CLIENTS
  logL = 0.0;
  std::vector<double> logLs;
  resetVector(num0Lineages);
  resetVector(num1Lineages);
  resetVector(num2Lineages);
  gather(world, logL, logLs, server);
  logL =  VectorTools::sum(logLs);
  gather(world, num0Lineages, allNum0Lineages, server);
  gather(world, num1Lineages, allNum1Lineages, server);
  gather(world, num2Lineages, allNum2Lineages, server);
  int temp = allNum0Lineages.size();
  for (int k =0; k<temp ; k++ ) {
    num0Lineages= num0Lineages+allNum0Lineages[k];
    num1Lineages= num1Lineages+allNum1Lineages[k];
    num2Lineages= num2Lineages+allNum2Lineages[k];
  }  
  return currentSpeciesTree;
}

/************************************************************************
 * Computes the likelihood of a species tree, optimizes the duplication and loss
 * rates, and updates the likelihood of the species tree. This way the likelihood
 * of this species tree is computed with adequate rates. 
 ************************************************************************/
void computeSpeciesTreeLikelihoodWhileOptimizingDuplicationAndLossRates(const mpi::communicator& world, 
                                                                        int &index, bool stop, double &logL, 
                                                                        std::vector<int> &num0Lineages, 
                                                                        std::vector<int> &num1Lineages, 
                                                                        std::vector<int> &num2Lineages, 
                                                                        std::vector< std::vector<int> > &allNum0Lineages, 
                                                                        std::vector< std::vector<int> > &allNum1Lineages, 
                                                                        std::vector< std::vector<int> > &allNum2Lineages, 
                                                                        std::vector<double> &lossExpectedNumbers, 
                                                                        std::vector<double> &duplicationExpectedNumbers, 
                                                                        bool rearrange, int server, 
                                                                        std::string &branchProbaOptimization, 
                                                                        std::map < std::string, int> genomeMissing, 
                                                                        TreeTemplate<Node> &tree, double & bestlogL) 
{
  if (branchProbaOptimization != "no")
    {
    //we set the expected numbers to 0.001 uniformly: we start afresh for each new species tree topology.
    for (int i = 0 ; i < lossExpectedNumbers.size() ; i++ ) 
      {
      lossExpectedNumbers[i] = 0.001;
      duplicationExpectedNumbers[i] = 0.001;
      }
    }
  std::string currentSpeciesTree = computeSpeciesTreeLikelihood(world, index, stop, logL, num0Lineages, num1Lineages,num2Lineages, allNum0Lineages, allNum1Lineages, allNum2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, rearrange, server, branchProbaOptimization, genomeMissing, tree);
  std::cout << "Species tree LogLikelihood after the first round: "<< - logL<<std::endl;
  double currentlogL = -UNLIKELY;
  int i=1;
  if (branchProbaOptimization != "no")
    {
    //Then we update duplication and loss rates based on the results of this first 
    //computation, until the likelihood stabilizes (roughly)
    while ((i<=1)&&(currentlogL-logL>logL-bestlogL)) 
      { 
        currentlogL = logL;
        i++;
        computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, tree);
        computeSpeciesTreeLikelihoodWithGivenStringSpeciesTree(world,index, stop, logL, num0Lineages, num1Lineages, num2Lineages, allNum0Lineages, allNum1Lineages, allNum2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, rearrange, server, branchProbaOptimization, genomeMissing, tree, currentSpeciesTree, false);      
      }
    }

  std::cout <<"\t\tNumber of species trees tried: "<<index<< std::endl;
  std::cout<<"\t\tLogLk value for this species tree: "<< - logL<<std::endl;

 // std::cout << i<< " iterations of likelihood computation for optimizing duplication and loss rates have been done."<< std::endl;
}




/******************************************************************************/
// This function runs the first communications between the server and the clients.
/******************************************************************************/
void firstCommunicationsServerClient (const mpi::communicator & world, int & server, std::vector <int>  & numbersOfGenesPerClient, int & assignedNumberOfGenes, std::vector<std::string> & assignedFilenames, std::vector <std::vector<std::string> > & listOfOptionsPerClient, bool & optimizeSpeciesTreeTopology, int & SpeciesNodeNumber, std::vector <double> & lossExpectedNumbers, std::vector <double> & duplicationExpectedNumbers, std::vector <int> & num0Lineages, std::vector <int> & num1Lineages, std::vector <int> & num2Lineages, std::string & currentSpeciesTree)
{
  //The server sends the number of genes assigned to each client
  scatter(world , numbersOfGenesPerClient, assignedNumberOfGenes, server);
  //Then the server sends each client the std::vector of filenames it is in charge of
  scatter(world , listOfOptionsPerClient, assignedFilenames, server);
  //Now the server sends to all clients the same information.
  broadcast(world, optimizeSpeciesTreeTopology, server);
  broadcast(world, SpeciesNodeNumber, server);
  broadcast(world, lossExpectedNumbers, server); 
  broadcast(world, duplicationExpectedNumbers, server); 
  broadcast(world, num0Lineages, server);
  broadcast(world, num1Lineages, server);
  broadcast(world, num2Lineages, server);
  broadcast(world, currentSpeciesTree, server);
  return;  
}




/******************************************************************************/
// These functions input and output alternate topologies likelihoods.
/******************************************************************************/

void inputNNIAndRootLks(std::vector <double> & NNILks, std::vector <double> & rootLks, std::map<std::string, std::string> & params, std::string & suffix)
{
  std::string file = ApplicationTools::getAFilePath("alternate.topology.likelihoods", params, false, false, suffix, true);

  std::ifstream in (file.c_str(), std::ios::in);

  std::string line;

  if (in.is_open())
    {

    int i=0;
    while ((! in.eof() )  && (i<NNILks.size() ) )
      {

      getline (in,line);
      StringTokenizer st = StringTokenizer::StringTokenizer (line, "\t", true, true);
      if (st.getToken(0) != "NNI Likelihoods")
        {
        std::cout <<"Inputing values for node "<<i<<std::endl;
        NNILks[i] = TextTools::toDouble(st.getToken(0));
        rootLks[i] = TextTools::toDouble(st.getToken(1));
        i = i+1;
        }
      }
    in.close();
    }
}


void outputNNIAndRootLks(std::vector <double> & NNILks, std::vector <double> & rootLks, std::map<std::string, std::string> & params, std::string & suffix)
{
  std::string file = ApplicationTools::getAFilePath("alternate.topology.likelihoods", params, false, false, suffix, true);
  std::ofstream out (file.c_str(), std::ios::out);
  out << "NNI Likelihoods"<<"\t"<<"Root likelihoods"<<std::endl;
  for (int i = 0 ; i < NNILks.size() ; i++)
    {
    out<<TextTools::toString(NNILks[i], 15)<<"\t"<<TextTools::toString(rootLks[i], 15)<<std::endl;
    }
  out.close();  
}








/**********************************CRAP******************************************/





/*  
 broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
 //COMPUTATION IN CLIENTS
 index++;  
 std::cout <<"\t\tNumber of species trees tried: "<<index<< std::endl;
 logL = 0.0;
 std::vector<double> logLs;
 resetVector(num0Lineages);
 resetVector(num1Lineages);
 resetVector(num2Lineages);
 gather(world, logL, logLs, server);
 
 logL =  VectorTools::sum(logLs);
 std::cout<<"New minus logLk value in computeSpeciesTreeLikelihood: "<<logL<<std::endl;
 gather(world, num0Lineages, allNum0Lineages, server);
 gather(world, num1Lineages, allNum1Lineages, server);
 gather(world, num2Lineages, allNum2Lineages, server);
 int temp = allNum0Lineages.size();
 for (int k =0; k<temp ; k++ ) {
 num0Lineages= num0Lineages+allNum0Lineages[k];
 num1Lineages= num1Lineages+allNum1Lineages[k];
 num2Lineages= num2Lineages+allNum2Lineages[k];
 }
 */





/*  std::string currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
 
 while (!stop) {
 std::vector<double> logLs;
 broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
 //Computation in clients
 index++;  
 std::cout <<"\tNumber of species trees tried : "<<index<< std::endl;
 logL = 0.0;
 resetVector(num0Lineages);
 resetVector(num1Lineages);
 resetVector(num2Lineages);
 gather(world, logL, logLs, server);
 logL =  VectorTools::sum(logLs);
 gather(world, num0Lineages, allNum0Lineages, server);
 gather(world, num1Lineages, allNum1Lineages, server);
 gather(world, num2Lineages, allNum2Lineages, server);
 int temp = allNum0Lineages.size();
 for (int k =0; k<temp ; k++ ) {
 num0Lineages= num0Lineages+allNum0Lineages[k];
 num1Lineages= num1Lineages+allNum1Lineages[k];
 num2Lineages= num2Lineages+allNum2Lineages[k];
 } 
 
 if (logL+0.01<bestlogL) {
 std::cout << "\t\tDuplication and loss probabilities optimization: Server : new total Likelihood value "<<logL<<" compared to the best log Likelihood : "<<bestlogL<< std::endl;
 std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
 numIterationsWithoutImprovement = 0;
 bestlogL =logL;
 if (bestTree) 	
 {
 deleteTreeProperties(*bestTree);
 delete bestTree;
 }
 bestTree = tree->clone();
 bestIndex = index;
 bestNum0Lineages = num0Lineages;
 bestNum1Lineages = num1Lineages;
 bestNum2Lineages = num2Lineages;
 
 computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
 breadthFirstreNumber (*tree, duplicationExpectedNumbers, lossExpectedNumbers);
 
 broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
 
 //COMPUTATION IN CLIENTS
 index++;  
 bestIndex = index;
 std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
 logL = 0.0;
 std::vector<double> logLs;
 resetVector(num0Lineages);
 resetVector(num1Lineages);
 resetVector(num2Lineages);
 gather(world, logL, logLs, server);
 logL =  VectorTools::sum(logLs);
 gather(world, num0Lineages, allNum0Lineages, server);
 gather(world, num1Lineages, allNum1Lineages, server);
 gather(world, num2Lineages, allNum2Lineages, server);
 temp = allNum0Lineages.size();
 for (int i =0; i<temp ; i++ ) {
 num0Lineages= num0Lineages+allNum0Lineages[i];
 num1Lineages= num1Lineages+allNum1Lineages[i];
 num2Lineages= num2Lineages+allNum2Lineages[i];
 }
 if ( (ApplicationTools::getTime() > timeLimit) ) {
 stop = true;
 broadcast(world, stop, server); 
 broadcast(world, bestIndex, server);
 }
 }
 else {
 numIterationsWithoutImprovement++;
 std::cout <<"\t\tNo more increase in likelihood "<< std::endl;
 deleteTreeProperties(*tree);
 delete tree;
 tree = bestTree->clone();
 stop = true;
 broadcast(world, stop, server); 
 broadcast(world, bestIndex, server);
 }
 }*/






/************************************************************************
 * Optimizes the duplication and loss rates with a numerical algorithm.
 * As we should start from overestimated values, we start by decreasing all 
 * values, and keep on trying to minimize these rates until the likelihood stops 
 * increasing. 
 ************************************************************************/
/*void numericalOptimizationOfDuplicationAndLossRates(const mpi::communicator& world, int &index, bool stop, double &logL, std::vector<int> &lossNumbers, std::vector<int> &duplicationNumbers, std::vector<int> &branchNumbers, std::vector< std::vector<int> > AllLosses, std::vector< std::vector<int> > AllDuplications, std::vector< std::vector<int> > AllBranches, std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, std::vector<int> &num2Lineages, std::vector< std::vector<int> > &allNum0Lineages, std::vector< std::vector<int> > &allNum1Lineages, std::vector< std::vector<int> > &allNum2Lineages, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, bool rearrange, int server, std::string &branchProbaOptimization, std::map < std::string, int> genomeMissing, TreeTemplate<Node> &tree, double & bestlogL) {
  std::cout<<"Numerical optimization of Duplication And Loss expected numbers of events"<<std::endl;
  std::cout<<"Before optimization: Duplications"<<std::endl;
  VectorTools::print(duplicationExpectedNumbers);  
  std::cout<<"Before optimization: Losses"<<std::endl;
  VectorTools::print(lossExpectedNumbers);
  
  std::vector <double> formerDupRates = duplicationExpectedNumbers;
  std::vector <double> formerLossRates = lossExpectedNumbers;
  
  std::vector <double> backupDupProba  = duplicationExpectedNumbers;
  std::vector <double> backupLossProba = lossExpectedNumbers;
  
  std::vector <double> newDupRates = duplicationExpectedNumbers;
  std::vector <double> newLossRates = lossExpectedNumbers;
  double currentlogL = -UNLIKELY;
  std::cout <<"HEHEHEHEHcurrentlogL: "<<currentlogL<<" logL: "<<logL<<std::endl;
  for (int i = 1 ; i<10 ; i++) {
    currentlogL = logL;
    double d = (double)i /10.0;
    newDupRates = formerDupRates * d;
    newLossRates = formerLossRates * d;
    
    computeSpeciesTreeLikelihood(world, index, stop, logL, num0Lineages, num1Lineages, num2Lineages, allNum0Lineages, allNum1Lineages, allNum2Lineages, newDupRates, newLossRates, rearrange, server, branchProbaOptimization, genomeMissing, tree);
    std::cout <<"i: "<<i<<" currentlogL: "<<currentlogL<<" logL: "<<logL<<std::endl;
    //If we have improved the log-likelihood
    if (currentlogL < bestlogL) {
      duplicationExpectedNumbers = newDupRates;
      lossExpectedNumbers = newLossRates;
      bestlogL = currentlogL;
      std::cout<<"Better logLikelihood! logL: "<< currentlogL <<std::endl;
      std::cout<<"After optimization: Duplications"<<std::endl;
      VectorTools::print(duplicationExpectedNumbers);  
      std::cout<<"After optimization: Losses"<<std::endl;
      VectorTools::print(lossExpectedNumbers);
    }
  }
  stop = true;
  broadcast(world, stop, server); 
  broadcast(world, index, server);
  
  return;  
  
}

*/












/************************************************************************
 * Randomly chooses the modification to be done, either SPR or changeRoot. The probabilities of the two events are a function of expectedFrequencySPR and expectedFrequencyChangeRoot.
************************************************************************/
/*
void makeRandomModifications(TreeTemplate<Node> &tree, int expectedFrequencyNNI, int expectedFrequencySPR, int expectedFrequencyChangeRoot) {
  //breadthFirstreNumber (tree);
  int tot = expectedFrequencyNNI + expectedFrequencySPR + expectedFrequencyChangeRoot;
  int ran=RandomTools::giveIntRandomNumberBetweenZeroAndEntry(tot);
  std::vector <int> allNodeIds;
  //TEMPORARY
  //ran = 0;
  if (ran < expectedFrequencyNNI) {//Make a NNI move
    //we benefit from the fact that the tree has been numbered with the lowest indices as closest to the root : we discard 0,1,2    
    for (int i = 3; i<tree.getNumberOfNodes(); i++) {
      allNodeIds.push_back(i);
    }
    ran = RandomTools::giveIntRandomNumberBetweenZeroAndEntry(allNodeIds.size())+3;
    makeNNI(tree, ran);
  }
  else if (ran < expectedFrequencySPR ) { //Make a SPR move ! we do not want to get the root as one of the two nodes involved
    Node * N = tree.getRootNode();
    std::vector <int> firstHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(0)));
    std::vector <int> secondHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(1)));
    
    if (secondHalfNodeIds.size()<firstHalfNodeIds.size()) {
      allNodeIds = firstHalfNodeIds;
      for (int i = 0 ; i< secondHalfNodeIds.size() ; i++ ) {
	allNodeIds.push_back(secondHalfNodeIds[i]);
      }
    }
    else {
      allNodeIds = secondHalfNodeIds;
      for (int i = 0 ; i< firstHalfNodeIds.size() ; i++ ) {
	allNodeIds.push_back(firstHalfNodeIds[i]);
      }
    }
    ran = RandomTools::giveIntRandomNumberBetweenZeroAndEntry(allNodeIds.size());
    int cutNodeId = allNodeIds[ran];
    //TEMPORARY
    //  cutNodeId = 2;
    std::vector <int> forbiddenIds = TreeTemplateTools::getNodesId(*(tree.getNode(cutNodeId)));
    std::vector <int> toRemove;
    for (int i = 0 ; i< allNodeIds.size() ; i++) {
      if (VectorTools::contains(forbiddenIds, allNodeIds[i])) {
	toRemove.push_back(i);
      }
    }
    sort(toRemove.begin(), toRemove.end(), cmp);
    for (int i = 0 ; i< toRemove.size() ; i++) {
      std::vector<int>::iterator vi = allNodeIds.begin();
      allNodeIds.erase(vi+toRemove[i]);
    }
    int ran2 = RandomTools::giveIntRandomNumberBetweenZeroAndEntry(allNodeIds.size());
    //TEMPORARY
    // ran2 = 0;
    int newBrotherId = allNodeIds[ran2];
    makeSPR(tree, cutNodeId, newBrotherId);
    }
  else { //Make a ChangeRoot move !
   std::cout << "CHANGE ROOT MOVE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<< std::endl;
    allNodeIds = tree.getNodesId();
    ran = RandomTools::giveIntRandomNumberBetweenZeroAndEntry(allNodeIds.size());
    changeRoot(tree, allNodeIds[ran]);
  }
}




















  /*
    std::vector <int> firstHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(0)));
    std::vector <int> secondHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(1)));
    
    if (secondHalfNodeIds.size()<firstHalfNodeIds.size()) {
    allNodeIds = firstHalfNodeIds;
    allNodeIds.insert ( secondHalfNodeIds.end(), firstHalfNodeIds.begin(), firstHalfNodeIds.end() );
    //  allNodeIds = firstHalfNodeIds;
   // for (int i = 0 ; i< secondHalfNodeIds.size() ; i++ ) {
   // allNodeIds.push_back(secondHalfNodeIds[i]);
   // }
  
   }
   else { 
   allNodeIds = secondHalfNodeIds;
   allNodeIds.insert ( firstHalfNodeIds.end(), secondHalfNodeIds.end(),secondHalfNodeIds.end()); 
   //      allNodeIds = secondHalfNodeIds;
   //   for (int i = 0 ; i< firstHalfNodeIds.size() ; i++ ) {
   //   allNodeIds.push_back(firstHalfNodeIds[i]);
   //   }
    
   }
  */




 //TEST
    /*  std::cout << TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;

    //Send the new std::vectors to compute the new likelihood of the best tree
    computeAverageDuplicationAndlossExpectedNumbersForAllBranches (num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers);
    broadcast(world, stop, server);
    broadcast(world, rearrange, server); 
    broadcast(world, lossExpectedNumbers, server); 
    broadcast(world, duplicationExpectedNumbers, server); 
    currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, false);
    broadcast(world, currentSpeciesTree, server);
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
   std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    */
    //END TEST












/************************************************************************
 * Tries all SPRs of the subtree starting in node nodeForSPR, and executes the one with the highest likelihood.
 ************************************************************************/
/*void tryAllPossibleSPRsAndMakeBestOne(const mpi::communicator& world, TreeTemplate<Node> *currentTree, TreeTemplate<Node> *bestTree,int nodeForSPR, int &index, int &bestIndex,  bool stop, double &logL, double &bestlogL, std::vector<int> &lossNumbers, std::vector<int> &duplicationNumbers, std::vector<int> &branchNumbers, std::vector<int> &bestLossNumbers, std::vector<int> &bestDuplicationNumbers, std::vector<int> &bestBranchNumbers, std::vector< std::vector<int> > AllLosses, std::vector< std::vector<int> > AllDuplications, std::vector< std::vector<int> > AllBranches, std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, std::vector<int> &num2Lineages, std::vector<int> &bestNum0Lineages, std::vector<int> &bestNum1Lineages, std::vector<int> &bestNum2Lineages, std::vector< std::vector<int> > &allNum0Lineages, std::vector< std::vector<int> > &allNum1Lineages, std::vector< std::vector<int> > &allNum2Lineages, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, double averageDuplicationProbability, double averageLossProbability, bool rearrange, int &numIterationsWithoutImprovement, int server, std::string &branchProbaOptimization, std::map < std::string, int> genomeMissing) {
  
  std::vector <double> backupDupProba=duplicationExpectedNumbers;
  std::vector<double> backupLossProba=lossExpectedNumbers;
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  TreeTemplate<Node> *tree;
  std::vector <int> nodeIdsToRegraft; 
  tree = currentTree->clone();
  buildVectorOfRegraftingNodes(*tree, nodeForSPR, nodeIdsToRegraft);
  bool betterTree = false;
  for (int i =0 ; i<nodeIdsToRegraft.size() ; i++) {
    if (i!=0) {
      deleteTreeProperties(*tree);
      delete tree;
      tree = currentTree->clone();
    }
    
    makeSPR(*tree, nodeForSPR, nodeIdsToRegraft[i]);
    duplicationExpectedNumbers = backupDupProba;
    lossExpectedNumbers = backupLossProba;
    breadthFirstreNumber (*tree, duplicationExpectedNumbers, lossExpectedNumbers);
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
       
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    //SHOULD WE USE ACCUMULATE ?
    int temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    
    //SECOND COMPUTATION, TO COMPUTE WITH GOOD RATES OF LOSSES AND DUPLICATION
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    resetVector(logLs);
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    
    //SHOULD WE USE ACCUMULATE ?
    temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    
    //END OF SECOND COMPUTATION
    
    std::cout<<"second computation ended"<< std::endl;
    
    if (logL+0.01<bestlogL) {
      betterTree = true;
      bestlogL =logL;
      deleteTreeProperties(*bestTree);
      delete bestTree;
      bestTree = tree->clone(); 
      bestDupProba=duplicationExpectedNumbers;
      bestLossProba=lossExpectedNumbers;
      bestLossNumbers = lossNumbers;
      bestDuplicationNumbers = duplicationNumbers;
      bestBranchNumbers = branchNumbers;
      bestNum0Lineages = num0Lineages;
      bestNum1Lineages = num1Lineages;
      bestNum2Lineages = num2Lineages;
      bestIndex = index;
      std::cout << "SPRs: Better candidate tree likelihood : "<<bestlogL<< std::endl;
      std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
    }
  }
  if (betterTree) {
    logL = bestlogL;
    duplicationExpectedNumbers = bestDupProba;
    lossExpectedNumbers = bestLossProba;
    numIterationsWithoutImprovement = 0;
    lossNumbers = bestLossNumbers;
    duplicationNumbers = bestDuplicationNumbers;
    branchNumbers = bestBranchNumbers;
    num0Lineages = bestNum0Lineages;
    num1Lineages = bestNum1Lineages;
    num2Lineages = bestNum2Lineages;
    deleteTreeProperties(*currentTree);
    delete currentTree;
    currentTree = bestTree->clone(); 
    std::cout <<"SPRs: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< std::endl;
    std::cout << "\t\tServer: SPRs: new total Likelihood value "<<logL<< std::endl;
    std::cout <<"Total Losses: "<<VectorTools::sum(lossNumbers) <<"; Total Duplications: "<<VectorTools::sum(duplicationNumbers)<< std::endl; 
    //Send the new std::vectors to compute the new likelihood of the best tree
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    breadthFirstreNumber (*currentTree);
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
    std::cout <<"bestIndex again "<<bestIndex<< std::endl;
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
  }
  else {
    numIterationsWithoutImprovement++;
    std::cout <<"SPRs: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< std::endl;
  }
  deleteTreeProperties(*tree);
  delete tree;
}

/************************************************************************
 * Tries all reRootings of the species tree, and executes the one with the highest likelihood.
 ************************************************************************/
/*void tryAllPossibleReRootingsAndMakeBestOne(const mpi::communicator& world, TreeTemplate<Node> *currentTree, TreeTemplate<Node> *bestTree, int &index, int &bestIndex,  bool stop, double &logL, double &bestlogL, std::vector<int> &lossNumbers, std::vector<int> &duplicationNumbers, std::vector<int> &branchNumbers, std::vector<int> &bestLossNumbers, std::vector<int> &bestDuplicationNumbers, std::vector<int> &bestBranchNumbers, std::vector< std::vector<int> > AllLosses, std::vector< std::vector<int> > AllDuplications, std::vector< std::vector<int> > AllBranches, std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, std::vector<int> &num2Lineages, std::vector<int> &bestNum0Lineages, std::vector<int> &bestNum1Lineages, std::vector<int> &bestNum2Lineages, std::vector< std::vector<int> > &allNum0Lineages, std::vector< std::vector<int> > &allNum1Lineages, std::vector< std::vector<int> > &allNum2Lineages, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, double averageDuplicationProbability, double averageLossProbability, bool rearrange, int &numIterationsWithoutImprovement, int server, std::string &branchProbaOptimization, std::map < std::string, int> genomeMissing) {
  
  std::vector <double> backupDupProba=duplicationExpectedNumbers;
  std::vector<double> backupLossProba=lossExpectedNumbers;
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  TreeTemplate<Node> *tree;
  tree = currentTree->clone();
  bool betterTree = false;
  std::vector <int> nodeIds = tree->getNodesId();
  for (int i =0 ; i<nodeIds.size() ; i++) {
    if (i!=0) {
      deleteTreeProperties(*tree);
      delete tree;
      tree = currentTree->clone();
    }
    changeRoot(*tree, nodeIds[i]);
    duplicationExpectedNumbers = backupDupProba;
    lossExpectedNumbers = backupLossProba;
    breadthFirstreNumber (*tree, duplicationExpectedNumbers, lossExpectedNumbers);  
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    //SHOULD WE USE ACCUMULATE ?
    int temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    //SECOND COMPUTATION, TO COMPUTE WITH GOOD RATES OF LOSSES AND DUPLICATION
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    breadthFirstreNumber (*tree, duplicationExpectedNumbers, lossExpectedNumbers);
    currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    resetVector(logLs);
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    
    //SHOULD WE USE ACCUMULATE ?
    temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    
    
    //END OF SECOND COMPUTATION
    
    if (logL+0.01<bestlogL) { 
      numIterationsWithoutImprovement = 0;
      betterTree = true;
      bestlogL =logL;
      if (bestTree) {
        delete bestTree;
      }
      bestTree = tree->clone(); 
      bestDupProba=duplicationExpectedNumbers;
      bestLossProba=lossExpectedNumbers;
      bestLossNumbers = lossNumbers;
      bestDuplicationNumbers = duplicationNumbers;
      bestBranchNumbers = branchNumbers; 
      bestNum0Lineages = num0Lineages;
      bestNum1Lineages = num1Lineages;
      bestNum2Lineages = num2Lineages;
      bestIndex = index;
      std::cout <<"ReRooting: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< " logLk: "<<logL<< std::endl;
      std::cout << "Better candidate tree likelihood : "<<bestlogL<< std::endl; 
      std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
    }
    else {
      numIterationsWithoutImprovement++;
      std::cout <<"ReRooting: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< " logLk: "<<logL<< std::endl;
    }
  }
  if (betterTree) {
    logL = bestlogL; 
    duplicationExpectedNumbers = bestDupProba;
    lossExpectedNumbers = bestLossProba;
    lossNumbers = bestLossNumbers;
    duplicationNumbers = bestDuplicationNumbers;
    branchNumbers = bestBranchNumbers;
    num0Lineages = bestNum0Lineages;
    num1Lineages = bestNum1Lineages;
    num2Lineages = bestNum2Lineages;
    delete currentTree;
    currentTree = bestTree->clone(); 
    std::cout << "\t\tServer: tryAllPossibleReRootingsAndMakeBestOne: new total Likelihood value "<<logL<< std::endl;
    std::cout << TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;
    
    
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    breadthFirstreNumber (*currentTree);
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    
    
  }
  deleteTreeProperties(*tree);
  delete tree;
  std::cout<<"tree deleted !!!"<< std::endl;
  
}


/************************************************************************
 * Tries all NNIs, and executes NNIs as soon as they increase the likelihood.
 ************************************************************************/
/*void tryAndMakeNNIs(const mpi::communicator& world, TreeTemplate<Node> *currentTree, TreeTemplate<Node> *bestTree, int &index, int &bestIndex,  bool stop, double &logL, double &bestlogL, std::vector<int> &lossNumbers, std::vector<int> &duplicationNumbers, std::vector<int> &branchNumbers, std::vector<int> &bestLossNumbers, std::vector<int> &bestDuplicationNumbers, std::vector<int> &bestBranchNumbers, std::vector< std::vector<int> > AllLosses, std::vector< std::vector<int> > AllDuplications, std::vector< std::vector<int> > AllBranches, std::vector<int> &num0Lineages, std::vector<int> &num1Lineages, std::vector<int> &num2Lineages, std::vector<int> &bestNum0Lineages, std::vector<int> &bestNum1Lineages, std::vector<int> &bestNum2Lineages,  std::vector< std::vector<int> > &allNum0Lineages, std::vector< std::vector<int> > &allNum1Lineages, std::vector< std::vector<int> > &allNum2Lineages, std::vector<double> &lossExpectedNumbers, std::vector<double> &duplicationExpectedNumbers, double averageDuplicationProbability, double averageLossProbability, bool rearrange, int &numIterationsWithoutImprovement, int server, std::string &branchProbaOptimization, std::map < std::string, int> genomeMissing) {
  
  std::vector <double> backupDupProba=duplicationExpectedNumbers;
  std::vector<double> backupLossProba=lossExpectedNumbers;
  std::vector <double> bestDupProba=duplicationExpectedNumbers;
  std::vector<double> bestLossProba=lossExpectedNumbers;
  TreeTemplate<Node> *tree;
  std::vector <int> nodeIds; 
  tree = currentTree->clone();
  nodeIds = currentTree->getNodesId();
  bool betterTree = false;
  for (int i =3 ; i<nodeIds.size() ; i++) {
    if (i!=0) {
      deleteTreeProperties(*tree);
      delete tree;
      tree = currentTree->clone();
    }
    makeNNI(*tree, i);
    duplicationExpectedNumbers = backupDupProba;
    lossExpectedNumbers = backupLossProba;
    breadthFirstreNumber (*tree, duplicationExpectedNumbers, lossExpectedNumbers);
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    //COMPUTATION IN CLIENTS
    index++;  
    
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    //SHOULD WE USE ACCUMULATE ?
    int temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    
    //SECOND COMPUTATION, TO COMPUTE WITH GOOD RATES OF LOSSES AND DUPLICATION
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *tree);
    }
    currentSpeciesTree = TreeTools::treeToParenthesis(*tree, true);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    resetVector(logLs);
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
    
    //SHOULD WE USE ACCUMULATE ?
    temp = AllDuplications.size();
    for (int k =0; k<temp ; k++ ) {
      duplicationNumbers= duplicationNumbers+AllDuplications[k];
      lossNumbers= lossNumbers+AllLosses[k];
      branchNumbers= branchNumbers+AllBranches[k];
      num0Lineages= num0Lineages+allNum0Lineages[i];
      num1Lineages= num1Lineages+allNum1Lineages[i];
      num2Lineages= num2Lineages+allNum2Lineages[i];
    } 
    
    
    //END OF SECOND COMPUTATION
    
    if (logL+0.01<bestlogL) {
      betterTree = true;    
      numIterationsWithoutImprovement = 0;
      bestlogL =logL;
      deleteTreeProperties(*bestTree);
      delete bestTree;
      bestTree = tree->clone();  
      bestDupProba=duplicationExpectedNumbers;
      bestLossProba=lossExpectedNumbers;
      bestLossNumbers = lossNumbers;
      bestDuplicationNumbers = duplicationNumbers;
      bestBranchNumbers = branchNumbers;
      bestNum0Lineages = num0Lineages;
      bestNum1Lineages = num1Lineages;
      bestNum2Lineages = num2Lineages;
      bestIndex = index;
      std::cout <<"tryAndMakeNNIs: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< std::endl;
      std::cout << "tryAndMakeNNIs: Better tree likelihood : "<<bestlogL<< std::endl; 
      std::cout << TreeTools::treeToParenthesis(*tree, true)<< std::endl;
    }
    else {
      numIterationsWithoutImprovement++;
      std::cout <<"NNI: Number of iterations without improvement : "<<numIterationsWithoutImprovement<< std::endl;
    }
  }
  if (betterTree) {
    logL = bestlogL; 
    duplicationExpectedNumbers = bestDupProba;
    lossExpectedNumbers = bestLossProba;
    lossNumbers = bestLossNumbers;
    duplicationNumbers = bestDuplicationNumbers;
    branchNumbers = bestBranchNumbers;
    num0Lineages = bestNum0Lineages;
    num1Lineages = bestNum1Lineages;
    num2Lineages = bestNum2Lineages;
    deleteTreeProperties(*currentTree);
    delete currentTree;
    currentTree = bestTree->clone(); 
    std::cout << "\t\tServer: tryAndMakeNNIs: new total Likelihood value "<<logL<< std::endl;
    std::cout << TreeTools::treeToParenthesis(*currentTree, true)<< std::endl;
    
    if ((branchProbaOptimization=="average")||(branchProbaOptimization=="average_then_branchwise")) {
      std::string temp = "average";
      computeDuplicationAndLossRatesForTheSpeciesTree (temp, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    else {
      computeDuplicationAndLossRatesForTheSpeciesTree (branchProbaOptimization, num0Lineages, num1Lineages, num2Lineages, lossExpectedNumbers, duplicationExpectedNumbers, genomeMissing, *currentTree);
    }
    std::string currentSpeciesTree = TreeTools::treeToParenthesis(*currentTree, true);
    breadthFirstreNumber (*currentTree);
    broadcastsAllInformation(world, server, stop, rearrange, lossExpectedNumbers, duplicationExpectedNumbers, currentSpeciesTree);
    //COMPUTATION IN CLIENTS
    index++;  
    bestIndex = index;
    std::cout <<"\t\tNumber of species trees tried : "<<index<< std::endl;
    logL = 0.0;
    std::vector<double> logLs;
    resetVector(duplicationNumbers);
    resetVector(lossNumbers);
    resetVector(branchNumbers);
    resetVector(num0Lineages);
    resetVector(num1Lineages);
    resetVector(num2Lineages);
    gather(world, logL, logLs, server);
    logL = VectorTools::sum(logLs);
    gather(world, duplicationNumbers, AllDuplications, server); 
    gather(world, lossNumbers, AllLosses, server);
    gather(world, branchNumbers, AllBranches, server);
    gather(world, num0Lineages, allNum0Lineages, server);
    gather(world, num1Lineages, allNum1Lineages, server);
    gather(world, num2Lineages, allNum2Lineages, server);
  }
  deleteTreeProperties(*tree);
  delete tree;
  
}

*/






/************************************************************************
 * Makes a modification, knowing what previous modifications have been done. Everything is deterministic, even SPRs.
 * For the moment we allow SPRs at a distance of 2 or 3 branches. There are 4 possible branching points at a distance of 2 branches, and 8 at a distance of 3 branches, which amounts to 12 branching points to try.
 
 ************************************************************************/
/*void makeTotallyDeterministicModifications(TreeTemplate<Node> &tree, int & nodeForNNI, int & nodeForSPR, int & nodeForRooting) {
  int numberOfBranchingPoints = 12;
  if (nodeForNNI < tree.getNumberOfNodes()) {//Make a NNI move
    if (nodeForNNI <3) {
      if (nodeForRooting<tree.getNumberOfNodes()) {
        changeRoot(tree, nodeForRooting);
        nodeForRooting++;
      }
      else {
        nodeForNNI=3;
        nodeForRooting = 1;
        makeNNI(tree, nodeForNNI);
        nodeForNNI++;
      }
    }
    else {
      makeNNI(tree, nodeForNNI);
      nodeForNNI++;
    }
  }
  else {//we make a SPR
    if (nodeForSPR == tree.getNumberOfNodes()) {
      nodeForSPR = 1;
      nodeForNNI = 0;
      changeRoot(tree, nodeForRooting);
      nodeForRooting++;
    }
    
    int reste = nodeForSPR % numberOfBranchingPoints;
    int prunedSubtree = nodeForSPR / numberOfBranchingPoints;
    
    
    
    std::vector <int> allNodeIds;
    Node * N = tree.getRootNode();
    std::vector <int> firstHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(0)));
    std::vector <int> secondHalfNodeIds = TreeTemplateTools::getNodesId(*(N->getSon(1)));
    
    if (secondHalfNodeIds.size()<firstHalfNodeIds.size()) {
      allNodeIds = firstHalfNodeIds;
      for (int i = 0 ; i< secondHalfNodeIds.size() ; i++ ) {
        allNodeIds.push_back(secondHalfNodeIds[i]);
      }
    }
    else {
      allNodeIds = secondHalfNodeIds;
      for (int i = 0 ; i< firstHalfNodeIds.size() ; i++ ) {
        allNodeIds.push_back(firstHalfNodeIds[i]);
      }
    }
    std::vector <int> forbiddenIds = TreeTemplateTools::getNodesId(*(tree.getNode(nodeForSPR)));
    
    int oldFatherId = tree.getNode(nodeForSPR)->getFather()->getId();
    int brotherId;
    //Get one brother ; a binary tree is supposed here (because of the "break")
    for(int i=0;i<tree.getNode(oldFatherId)->getNumberOfSons();i++)
      if(tree.getNode(oldFatherId)->getSon(i)->getId()!=nodeForSPR){brotherId=tree.getNode(oldFatherId)->getSon(i)->getId(); break;}
    if ((tree.getNode(oldFatherId)->hasFather())||(!tree.getNode(brotherId)->isLeaf())) {
      forbiddenIds.push_back(brotherId);
    }
    std::vector <int> toRemove;
    for (int i = 0 ; i< allNodeIds.size() ; i++) {
      if (VectorTools::contains(forbiddenIds, allNodeIds[i])) {
        toRemove.push_back(i);
      }
    }
    sort(toRemove.begin(), toRemove.end(), cmp);
    for (int i = 0 ; i< toRemove.size() ; i++) {
      std::vector<int>::iterator vi = allNodeIds.begin();
      allNodeIds.erase(vi+toRemove[i]);
    }
    int ran2 = RandomTools::giveIntRandomNumberBetweenZeroAndEntry(allNodeIds.size());
    int newBrotherId = allNodeIds[ran2];
    makeSPR(tree, nodeForSPR, newBrotherId);
    nodeForSPR++;
  }
  
}
*/


