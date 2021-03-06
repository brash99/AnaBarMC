#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"
#include "TDatabasePDG.h"
#include "TPDGCode.h"
#include "TF1.h"
#include "TH1.h"
#include "TBenchmark.h"
#include "TSystem.h"
#include "TMath.h"

#include <iostream>

// ------------------------------------------------------------------------------------------------

// Functions
void  InitOutput();
void  GenerateOneMuon();

// Random number generator
TRandom3*       fRand;

// PDG Database
TDatabasePDG*   fPDG;

// Output for G4
TFile*          fROOTFile;
TTree*          fROOTTree;
TString         fOutFileName;
Float_t         fVx;
Float_t         fVy;
Float_t         fVz;
Float_t         fPx;
Float_t         fPy;
Float_t         fPz;
Float_t         fP;
Float_t         fM;
Float_t         fE;
Int_t           fPDGCode;

// Sampling Functions
TH1*            fMomFlatDist;
TH1*            fMomPowDist;
TH1*            fThetaDist;
TH1*            fPhiDist;
const Float_t   fMomMin  =  0.5;
const Float_t   fMomMax  = 20.0;
const Float_t   fMomMean =  3.5;  
Float_t         fIntRatio;

// ------------------------------------------------------------------------------------------------

void GenCosmics( ULong64_t nevents = 100000, 
		 TString fname = "data/Gen_test1.root" ) 
{
  
  // Initialise random number generator
  fRand = new TRandom3( -1 );
  
  // Set up PDG Table
  fPDG             = new TDatabasePDG();
  TString pdgtable = gSystem->Getenv( "ROOTSYS" );
  pdgtable.Append( "/etc/pdg_table.txt" );
  fPDG->ReadPDGTable( pdgtable );

  // Initialise output
  fOutFileName = fname;
  InitOutput();

  // Initialise sampling functions
  TF1*    momPowFunc  = new TF1("momPowFunc", "x^(-2.7)", fMomMean, fMomMax );
  Float_t meanval     = momPowFunc->Eval( fMomMean );
  Char_t* meanstr     = new Char_t[sizeof("*******")+1];
  sprintf( meanstr, "%1.6f", meanval );
  TF1*    momFlatFunc = new TF1("momFlatFunc", meanstr, fMomMin, fMomMean );
  fIntRatio           = momFlatFunc->Integral( fMomMin, fMomMean ) / 
    ( momPowFunc->Integral( fMomMean, fMomMax ) + momFlatFunc->Integral(fMomMin, fMomMean ) );
  fMomFlatDist   = (TH1*)momFlatFunc->GetHistogram()->Clone("MomFlatDist");
  fMomPowDist    = (TH1*)momPowFunc->GetHistogram()->Clone("MomPowDist");

  TF1* thetaFunc = new TF1("thetaFunc", "cos(x) * cos(x)", TMath::Pi()/2., TMath::Pi()    );
  TF1* phiFunc   = new TF1("phiFunc",   "1",               0.,             TMath::TwoPi() );
  fThetaDist     = (TH1*)thetaFunc->GetHistogram()->Clone("ThetaDist");
  fPhiDist       = (TH1*)phiFunc->GetHistogram()->Clone("PhiDist");

  // Initialise counters
  ULong64_t   nTotal = 0;  
  TBenchmark* bench  = new TBenchmark();
  bench->Start("Statistics");
  
  // Main event loop
  for( ULong64_t i = 0; i < nevents; i++ ) 
    {
      nTotal++;
      
      GenerateOneMuon();
      fROOTTree->Fill();
      
      if( i % 100000 == 0 )
	cout << i << endl;
    }
  
  // Write output and close file
  fROOTTree->Write();
  fROOTFile->Close();
  
  // Print stats
  bench->Stop("Statistics");
  cout << "\t" <<  nTotal << " total events" << endl;
  bench->Print("Statistics");
  
}

// ------------------------------------------------------------------------------------------------

void InitOutput()
{
  fROOTFile = new TFile(fOutFileName, "RECREATE", "fROOTfile", 1);
  fROOTTree = new TTree("h1", "Generator Output Tree");
  fROOTTree->SetAutoSave();
  const Int_t basket = 64000;
  
  fROOTTree->Branch("X_vtx",   &fVx, "X_vtx/F", basket );
  fROOTTree->Branch("Y_vtx",   &fVy, "Y_vtx/F", basket );
  fROOTTree->Branch("Z_vtx",   &fVz, "Z_vtx/F", basket );
  
  fROOTTree->Branch("Px_p",    &fPx, "Px_p/F",  basket );
  fROOTTree->Branch("Py_p",    &fPy, "Py_p/F",  basket );
  fROOTTree->Branch("Pz_p",    &fPz, "Pz_p/F",  basket );
  fROOTTree->Branch("En_p",    &fE,  "En_p/F",  basket );

  fROOTTree->Branch("PDG", &fPDGCode, "PDG/I",  basket );

}

// ------------------------------------------------------------------------------------------------

void GenerateOneMuon()
{
  fPDGCode = 13;

  // Generate vertex position in cm 
  fVx = fRand->Uniform(-1.3 , 1.3 );
  fVy = fRand->Uniform( -2.0, 2.0 );
  //fVx = fRand->Uniform(-0.01 , 0.01 );
  //fVy = fRand->Uniform( -.01, 0.01 );
  fVz = 2.0;

  // Sample Momentum Distributions (flat from min to mean, p^-2.7 from mean to max)
  if( fRand->Uniform(0.,1) < fIntRatio ) 
    fP = 1000. * fMomFlatDist->GetRandom();
  else 
    fP = 1000. * fMomPowDist->GetRandom();

  // Sample Angular Distributions (cos^2(theta) and flat phi)
  Float_t th = fThetaDist->GetRandom();
  Float_t ph = fPhiDist->GetRandom();
  //Float_t th = 3.14159265;
  //Float_t ph = 0.0;
  fPx        = fP * TMath::Sin(th) * TMath::Cos(ph);
  fPy        = fP * TMath::Sin(th) * TMath::Sin(ph);
  fPz        = fP * TMath::Cos(th);
  fM         = fPDG->GetParticle( fPDGCode )->Mass() * 1000;
  fE         = TMath::Sqrt( (fP*fP + fM*fM) );
  
}

// ------------------------------------------------------------------------------------------------
