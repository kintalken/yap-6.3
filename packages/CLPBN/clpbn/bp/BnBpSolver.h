#ifndef HORUS_BNBPSOLVER_H
#define HORUS_BNBPSOLVER_H

#include <vector>
#include <set>

#include "Solver.h"
#include "BayesNet.h"
#include "Shared.h"

using namespace std;

class BpNodeInfo;

static const string PI_SYMBOL = "pi" ;
static const string LD_SYMBOL = "ld" ;

enum LinkOrientation  {UP, DOWN};
enum JointCalcType    {CHAIN_RULE, JUNCTION_NODE};

class BpLink
{
  public:
    BpLink (BayesNode* s, BayesNode* d, LinkOrientation o)
    { 
      source_      = s;
      destin_      = d;
      orientation_ = o;
      if (orientation_ == LinkOrientation::DOWN) {
        v1_.resize (s->nrStates(), Util::tl (1.0/s->nrStates()));
        v2_.resize (s->nrStates(), Util::tl (1.0/s->nrStates()));
      } else {
        v1_.resize (d->nrStates(), Util::tl (1.0/d->nrStates()));
        v2_.resize (d->nrStates(), Util::tl (1.0/d->nrStates()));
      }
      currMsg_   = &v1_;
      nextMsg_   = &v2_;
      residual_  = 0;
      msgSended_ = false;
    }

    void updateMessage (void)
    {
      swap (currMsg_, nextMsg_);
      msgSended_ = true;
    }
 
    void updateResidual (void)
    {
      residual_ = Util::getMaxNorm (v1_, v2_);
    }
   
    string toString (void) const
    {
      stringstream ss;
      if (orientation_ == LinkOrientation::DOWN) {
        ss << PI_SYMBOL;
      } else {
        ss << LD_SYMBOL;
      }
      ss << "(" << source_->label();
      ss << " --> " << destin_->label() << ")" ;
      return ss.str();
    }

    string toString (unsigned stateIndex) const
    {
      stringstream ss;
      ss << toString() << "[" ;
      if (orientation_ == LinkOrientation::DOWN) {
        ss << source_->states()[stateIndex] << "]" ;
      } else {
        ss << destin_->states()[stateIndex] << "]" ;
      }
      return ss.str();
    }

    BayesNode*      getSource (void) const        { return source_;        }
    BayesNode*      getDestination (void) const   { return destin_;        }
    LinkOrientation getOrientation (void) const   { return orientation_;   }
    const ParamSet& getMessage (void) const       { return *currMsg_;      }
    ParamSet&       getNextMessage (void)         { return *nextMsg_;      }
    bool            messageWasSended (void) const { return msgSended_;     }
    double          getResidual (void) const      { return residual_;      }
    void            clearResidual (void)          { residual_ = 0;}
 
  private:
    BayesNode*       source_;
    BayesNode*       destin_;
    LinkOrientation  orientation_;
    ParamSet         v1_;
    ParamSet         v2_;
    ParamSet*        currMsg_;
    ParamSet*        nextMsg_;
    bool             msgSended_;
    double           residual_;
};


typedef vector<BpLink*> BpLinkSet;


class BpNodeInfo
{
  public:
    BpNodeInfo (BayesNode*);

    ParamSet   getBeliefs (void) const;
    bool       receivedBottomInfluence (void) const;

    ParamSet&  getPiValues (void)                   { return piVals_;     }
    ParamSet&  getLambdaValues (void)               { return ldVals_;     }
    void       incNumPiMsgsReceived (void)          { nPiMsgsRcv_ ++;     }
    void       incNumLambdaMsgsReceived (void)      { nLdMsgsRcv_ ++;     }
    bool       piValuesCalculated (void)            { return piValsCalc_; }
    bool       lambdaValuesCalculated (void)        { return ldValsCalc_; }

    void markPiValuesAsCalculated (void);
    void markLambdaValuesAsCalculated (void);
    bool receivedAllPiMessages (void);
    bool receivedAllLambdaMessages (void);
    bool readyToSendPiMsgTo (const BayesNode*) const ;
    bool readyToSendLambdaMsgTo (const BayesNode*) const;

    const BpLinkSet& getIncomingParentLinks (void)  { return inParentLinks_;  }
    const BpLinkSet& getIncomingChildLinks (void)   { return inChildLinks_;   }
    const BpLinkSet& getOutcomingParentLinks (void) { return outParentLinks_; }
    const BpLinkSet& getOutcomingChildLinks (void)  { return outChildLinks_;  }
   
    void addIncomingParentLink  (BpLink* l)  { inParentLinks_.push_back (l);  }
    void addIncomingChildLink   (BpLink* l)  { inChildLinks_.push_back (l);   }
    void addOutcomingParentLink (BpLink* l)  { outParentLinks_.push_back (l); }
    void addOutcomingChildLink  (BpLink* l)  { outChildLinks_.push_back (l);  }
   
  private:
    DISALLOW_COPY_AND_ASSIGN (BpNodeInfo);

    ParamSet               piVals_;     // pi values
    ParamSet               ldVals_;     // lambda values
    unsigned               nPiMsgsRcv_;
    unsigned               nLdMsgsRcv_;
    bool                   piValsCalc_;
    bool                   ldValsCalc_;
    BpLinkSet              inParentLinks_;
    BpLinkSet              inChildLinks_;
    BpLinkSet              outParentLinks_;
    BpLinkSet              outChildLinks_;
    const BayesNode*       node_;
};



class BnBpSolver : public Solver
{
  public:
    BnBpSolver (const BayesNet&);
   ~BnBpSolver (void);

    void                runSolver (void);
    ParamSet            getPosterioriOf (VarId);
    ParamSet            getJointDistributionOf (const VarIdSet&);
  

  private:
    DISALLOW_COPY_AND_ASSIGN (BnBpSolver);

    void                initializeSolver (void);
    void                runPolyTreeSolver (void);
    void                runLoopySolver (void);
    void                maxResidualSchedule (void);
    bool                converged (void) const;
    void                updatePiValues (BayesNode*);
    void                updateLambdaValues (BayesNode*);
    void                calculateLambdaMessage (BpLink*);
    void                calculatePiMessage (BpLink*);
    ParamSet            getJointByJunctionNode (const VarIdSet&);
    ParamSet            getJointByChainRule (const VarIdSet&) const;
    void                printPiLambdaValues (const BayesNode*) const;
    void                printAllMessageStatus (void) const;

    void calculateAndUpdateMessage (BpLink* link, bool calcResidual = true)
    {
      if (DL >= 3) {
        cout << "calculating & updating " << link->toString() << endl;
      }
      if (link->getOrientation() == LinkOrientation::DOWN) {
        calculatePiMessage (link);
      } else if (link->getOrientation() == LinkOrientation::UP) {
        calculateLambdaMessage (link);
      }
      if (calcResidual) {
        link->updateResidual();
      }
      link->updateMessage();
    }

    void calculateMessage (BpLink* link, bool calcResidual = true)
    {
      if (DL >= 3) {
        cout << "calculating " << link->toString() << endl;
      }
      if (link->getOrientation() == LinkOrientation::DOWN) {
        calculatePiMessage (link);
      } else if (link->getOrientation() == LinkOrientation::UP) {
        calculateLambdaMessage (link);
      }
      if (calcResidual) {
        link->updateResidual();
      }
    }

    void updateMessage (BpLink* link)
    {
      if (DL >= 3) {
        cout << "updating " << link->toString() << endl;
      }
      link->updateMessage();
    }

    void updateValues (BpLink* link)
    {
      if (!link->getDestination()->hasEvidence()) {
        if (link->getOrientation() == LinkOrientation::DOWN) {
          updatePiValues (link->getDestination());
        } else if (link->getOrientation() == LinkOrientation::UP) {
          updateLambdaValues (link->getDestination());
        }
      }
    }

    BpNodeInfo* ninf (const BayesNode* node) const
    {
      assert (node);
      assert (node == bayesNet_->getBayesNode (node->varId()));
      assert (node->getIndex() < nodesI_.size());
      return nodesI_[node->getIndex()];
    }

    const BayesNet*      bayesNet_;
    vector<BpLink*>      links_;
    vector<BpNodeInfo*>  nodesI_;
    unsigned             nIters_;
    JointCalcType        jointCalcType_;

    struct compare
    {
      inline bool operator() (const BpLink* e1, const BpLink* e2)
      {
        return e1->getResidual() > e2->getResidual();
      }
    };

    typedef multiset<BpLink*, compare> SortedOrder;
    SortedOrder sortedOrder_;

    typedef unordered_map<BpLink*, SortedOrder::iterator> BpLinkMap;
    BpLinkMap linkMap_;

};

#endif // HORUS_BNBPSOLVER_H
