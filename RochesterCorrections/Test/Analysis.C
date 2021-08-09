#include "Analysis.h"
#include "TFile.h"
#include "TTree.h"
#include "Math/Vector4D.h"
#include "TBranch.h"

// Compute the invariant mass of two muon four-vectors
float computeInvariantMass(Float_t pt1, Float_t pt2, Float_t eta1, Float_t eta2, Float_t phi1, Float_t phi2, Float_t mass1, Float_t mass2) {
  ROOT::Math::PtEtaPhiMVector m1(pt1, eta1, phi1, mass1);
  ROOT::Math::PtEtaPhiMVector m2(pt2, eta2, phi2, mass2);
  return (m1 + m2).mass();
}

// Apply the corrections to dataset
int applyCorrections(string filename, string pathToFile, string treeName, bool isData, bool correctAll) {
  // Create TTree from ROOT file
  TFile *f1 = TFile::Open((pathToFile).c_str());
  TTree *DataTree = (TTree*)f1->Get((treeName).c_str());

  //Variables to hold values read from the tree
  int maxmuon=1000;
  UInt_t nMuon = 0;
  Float_t Muon_pt[maxmuon];
  Float_t Muon_eta[maxmuon];
  Float_t Muon_phi[maxmuon];
  Float_t Muon_mass[maxmuon];
  Int_t Muon_charge[maxmuon];

  //Set addresses to make the tree populate the variables when reading an entry
  DataTree->SetBranchAddress("nMuon", &nMuon);
  DataTree->SetBranchAddress("Muon_pt", &Muon_pt);
  DataTree->SetBranchAddress("Muon_eta", &Muon_eta);
  DataTree->SetBranchAddress("Muon_phi", &Muon_phi);
  DataTree->SetBranchAddress("Muon_mass", &Muon_mass);
  DataTree->SetBranchAddress("Muon_charge", &Muon_charge);

  //Create an output file for corrected values and a clone of the original tree
  TFile *f2 = new TFile(("./RochesterCorrections/Test/" + filename + "_Cor.root").c_str(),"recreate");
  TTree *DataTreeCor = DataTree->CloneTree(0);

  // New branches for corrected values
  Float_t Muon_pt_cor[maxmuon];
  Float_t Muon_eta_cor[maxmuon];
  Float_t Muon_phi_cor[maxmuon];
  Float_t Muon_mass_cor[maxmuon];

  TBranch *bMuon_pt_cor = DataTreeCor->Branch("Muon_pt_cor", &Muon_pt_cor, "Muon_pt_cor[nMuon]/F");
  TBranch *bMuon_eta_cor = DataTreeCor->Branch("Muon_eta_cor", &Muon_eta_cor, "Muon_eta_cor[nMuon]/F");
  TBranch *bMuon_phi_cor = DataTreeCor->Branch("Muon_phi_cor", &Muon_phi_cor, "Muon_phi_cor[nMuon]/F");
  TBranch *bMuon_mass_cor = DataTreeCor->Branch("Muon_mass_cor", &Muon_mass_cor, "Muon_mass_cor[nMuon]/F");

  // New branches for invariant mass of dimuon system
  Float_t Dimuon_mass;
  Float_t Dimuon_mass_cor;

  TBranch *bDimuon_mass = DataTreeCor->Branch("Dimuon_mass", &Dimuon_mass, "Dimuon_mass/F");
  TBranch *bDimuon_mass_cor = DataTreeCor->Branch("Dimuon_mass_cor", &Dimuon_mass_cor, "Dimuon_mass_cor/F");

  // New branches for plotting separately postive and negative muons' eta
  Float_t Muon_eta_pos;
  Float_t Muon_eta_neg;

  TBranch *bMuon_eta_pos = DataTreeCor->Branch("Muon_eta_pos", &Muon_eta_pos, "Muon_eta_pos/F");
  TBranch *bMuon_eta_neg = DataTreeCor->Branch("Muon_eta_neg", &Muon_eta_neg, "Muon_eta_neg/F");

  rochcor2012 rmcor; // make the pointer of rochcor class

  // Variables needed for corrections
  float ntrk = 0; //ntrk (number of track layer) is one of input and it can slightly improved the extra smearing
  float runopt = 0; //No run dependence for 2012 data, so default of “runopt=0”
  float qter = 1.0; // added by Higgs group’s request to propagate the uncertainty

  std::cout << "Looping over events" << std::endl;

  // Loop over events
  Int_t nEntries = (Int_t)DataTree->GetEntries();

  for (Int_t k=0; k<nEntries; k++) {
    DataTree->GetEntry(k);

    if (correctAll) { // Correct all muons in dataset
      if (nMuon > 0) {
        // Loop over muons in event
        for (UInt_t i=0; i<nMuon; i++) {
          // Create TLorentzVector
          TLorentzVector mu;
          mu.SetPtEtaPhiM(Muon_pt[i], Muon_eta[i], Muon_phi[i], Muon_mass[i]);

          // Apply the corrections
          if (isData) {
            rmcor.momcor_data(mu, Muon_charge[i], runopt, qter);
          } else {
            rmcor.momcor_mc(mu, Muon_charge[i], ntrk, qter);
          }

          // Save corrected values
          Muon_pt_cor[i] = mu.Pt();
          Muon_eta_cor[i] = mu.Eta();
          Muon_phi_cor[i] = mu.Phi();
          Muon_mass_cor[i] = mu.M();
        }

        DataTreeCor->Fill();
      }
    } else { // Correct muons that pass the selections
      // Select events with exactly two muons
      if (nMuon == 2 ) {
        // Select events with two muons of opposite charge
        if (Muon_charge[0] != Muon_charge[1]) {

          // Compute invariant mass of the dimuon system
          Dimuon_mass = computeInvariantMass(Muon_pt[0], Muon_pt[1], Muon_eta[0], Muon_eta[1], Muon_phi[0], Muon_phi[1], Muon_mass[0], Muon_mass[1]);

          // Choose positive and negative muons' etas
          if (Muon_charge[0] > 0) {
            Muon_eta_pos = Muon_eta[0];
            Muon_eta_neg = Muon_eta[1];
          } else {
            Muon_eta_pos = Muon_eta[1];
            Muon_eta_neg = Muon_eta[0];
          }

          // Loop over muons in event
          for (UInt_t i=0; i<nMuon; i++) {
            // Create TLorentzVector
            TLorentzVector mu;
            mu.SetPtEtaPhiM(Muon_pt[i], Muon_eta[i], Muon_phi[i], Muon_mass[i]);

            // Apply the corrections
            if (isData) {
              rmcor.momcor_data(mu, Muon_charge[i], runopt, qter);
            } else {
              rmcor.momcor_mc(mu, Muon_charge[i], ntrk, qter);
            }

            // Save corrected values
            Muon_pt_cor[i] = mu.Pt();
            Muon_eta_cor[i] = mu.Eta();
            Muon_phi_cor[i] = mu.Phi();
            Muon_mass_cor[i] = mu.M();
          }

          // Compute invariant mass of the corrected dimuon system
          Dimuon_mass_cor = computeInvariantMass(Muon_pt_cor[0], Muon_pt_cor[1], Muon_eta_cor[0], Muon_eta_cor[1], Muon_phi_cor[0], Muon_phi_cor[1], Muon_mass_cor[0], Muon_mass_cor[1]);

          DataTreeCor->Fill();

        }
      }
    }
  }

  std::cout << "Writing tree to ouput file" << std::endl;

  //Save the new tree
  DataTreeCor->Write();

  delete f1;
  delete f2;

  return 0;
}

void Analysis::main()
{
  // Example
  // applyCorrections("nameOfFile", "pathToFile", "treeName", isData, correctAll);

  // Data
  applyCorrections("Run2012BC_DoubleMuParked_Muons", "root://eospublic.cern.ch//eos/opendata/cms/derived-data/AOD2NanoAODOutreachTool/Run2012BC_DoubleMuParked_Muons.root", "Events", true, false);

  // MC
  applyCorrections("ZZTo2e2mu", "root://eospublic.cern.ch//eos/opendata/cms/upload/stefan/HiggsToFourLeptonsNanoAODOutreachAnalysis/ZZTo2e2mu.root", "Events", false, false);
}
