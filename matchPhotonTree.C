#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TGraphAsymmErrors.h>
#include <TH1D.h>
#include <iostream>
#include <TMath.h>

#include "EventMatchingCMS.h"

const int MAXPHOTONS = 500;
const int nBins = 100;
const double maxPt = 100;
const float offlineEtaCut = 1.44;
const bool offlineIso = false;

void matchPhotonTree(TString HiForestName, TString bitfileName, TString outFileName)
{
  TFile *HLTFile = TFile::Open(bitfileName);
  TTree *HLTTree = (TTree*)HLTFile->Get("hltbitanalysis/HltTree");

  ULong64_t hlt_event;
  Int_t hlt_run, hlt_lumi;

  const int NTRIG = 12;
  TString trigname[NTRIG] = {"HLT_HISinglePhoton10_v1",
			     "HLT_HISinglePhoton15_v1",
			     "HLT_HISinglePhoton20_v1",
			     "HLT_HISinglePhoton30_v1",
			     "HLT_HISinglePhoton40_v1",
			     "HLT_HISinglePhoton50_v1",
			     "HLT_HISinglePhoton60_v1",
			     "L1_SingleEG3_BptxAND",
			     "L1_SingleEG7_BptxAND",
			     "L1_SingleEG12_BptxAND",
			     "L1_SingleEG21_BptxAND",
			     "L1_SingleEG30_BptxAND"};

  Int_t triggers[NTRIG];

  HLTTree->SetBranchAddress("Event",&hlt_event);
  HLTTree->SetBranchAddress("Run",&hlt_run);
  HLTTree->SetBranchAddress("LumiBlock",&hlt_lumi);

  for(int i = 0; i < NTRIG; i++)
  {
    HLTTree->SetBranchAddress(trigname[i], &(triggers[i]));
  }

  TFile *inFile = TFile::Open(HiForestName);
  TTree *f1Tree = (TTree*)inFile->Get("multiPhotonAnalyzer/photon");
  TTree *fEvtTree = (TTree*)inFile->Get("hiEvtAnalyzer/HiTree");
  TTree *fSkimTree = (TTree*)inFile->Get("skimanalysis/HltTree");

  Int_t f_evt, f_run, f_lumi;
  Float_t vz;
  Int_t hiBin;
  fEvtTree->SetBranchAddress("evt",&f_evt);
  fEvtTree->SetBranchAddress("run",&f_run);
  fEvtTree->SetBranchAddress("lumi",&f_lumi);
  fEvtTree->SetBranchAddress("vz",&vz);
  fEvtTree->SetBranchAddress("hiBin",&hiBin);

  Int_t pcollisionEventSelection, pHBHENoiseFilter;
  fSkimTree->SetBranchAddress("pcollisionEventSelection",&pcollisionEventSelection);
  fSkimTree->SetBranchAddress("pHBHENoiseFilter",&pHBHENoiseFilter);

  Int_t nPhoton;
  Float_t photon_pt[MAXPHOTONS];
  Float_t photon_eta[MAXPHOTONS];
  Float_t photon_phi[MAXPHOTONS];
  Float_t cc4[MAXPHOTONS];
  Float_t cr4[MAXPHOTONS];
  Float_t ct4PtCut20[MAXPHOTONS];
  Float_t trkSumPtHollowConeDR04[MAXPHOTONS];
  Float_t hcalTowerSumEtConeDR04[MAXPHOTONS];
  Float_t ecalRecHitSumEtConeDR04[MAXPHOTONS];
  Float_t hadronicOverEm[MAXPHOTONS];
  Float_t sigmaIetaIeta[MAXPHOTONS];
  Int_t isEle[MAXPHOTONS];
  Float_t sigmaIphiIphi[MAXPHOTONS];
  Float_t swissCrx[MAXPHOTONS];
  Float_t seedTime[MAXPHOTONS];

  f1Tree->SetBranchAddress("nPhotons",&nPhoton);
  f1Tree->SetBranchAddress("pt",photon_pt);
  f1Tree->SetBranchAddress("eta",photon_eta);
  f1Tree->SetBranchAddress("phi",photon_phi);

  f1Tree->SetBranchAddress("cc4",cc4);
  f1Tree->SetBranchAddress("cr4",cr4);
  f1Tree->SetBranchAddress("ct4PtCut20",ct4PtCut20);
  f1Tree->SetBranchAddress("trkSumPtHollowConeDR04",trkSumPtHollowConeDR04);
  f1Tree->SetBranchAddress("hcalTowerSumEtConeDR04",hcalTowerSumEtConeDR04);
  f1Tree->SetBranchAddress("ecalRecHitSumEtConeDR04",ecalRecHitSumEtConeDR04);
  f1Tree->SetBranchAddress("hadronicOverEm",hadronicOverEm);
  f1Tree->SetBranchAddress("sigmaIetaIeta",sigmaIetaIeta);
  f1Tree->SetBranchAddress("isEle",isEle);
  f1Tree->SetBranchAddress("sigmaIphiIphi",sigmaIphiIphi);
  f1Tree->SetBranchAddress("swissCrx",swissCrx);
  f1Tree->SetBranchAddress("seedTime",seedTime);

  TH1D *fPt = new TH1D("fPt_0",";offline p_{T} (GeV)",nBins,0,maxPt);
  TH1D *accepted[NTRIG];

  for(int i = 0; i < NTRIG; ++i)
  {
    accepted[i] = new TH1D(Form("accepted_pt%d",i),";offline p_{T}",nBins,0,maxPt);
  }

  // Make the event-matching map ************
  EventMatchingCMS *matcher = new EventMatchingCMS();
  int duplicates = 0;
  std::cout << "Begin making map." << std::endl;
  Long64_t l_entries = HLTTree->GetEntries();
  for(Long64_t j = 0; j < l_entries; ++j)
  {
    HLTTree->GetEntry(j);
    bool status = matcher->addEvent(hlt_event, hlt_lumi, hlt_run, j);
    if(status == false)
      duplicates++;
  }
  std::cout << "Finished making map." << std::endl;
  std::cout << "Duplicate entries: " << duplicates << std::endl;
  // **********************

  // analysis loop
  int matched = 0;
  for(Long64_t entry = 0; entry < fEvtTree->GetEntries(); ++entry)
  {
    fEvtTree->GetEntry(entry);
    long long hlt_entry = matcher->retrieveEvent(f_evt, f_lumi, f_run);
    if(hlt_entry == -1) continue;
    matched++;

    fSkimTree->GetEntry(entry);

    bool goodEvent = false;
    if((pcollisionEventSelection == 1) && (TMath::Abs(vz) < 15))
    {
      goodEvent = true;
    }
    if(!goodEvent) continue;

    f1Tree->GetEntry(entry);
    HLTTree->GetEntry(hlt_entry);

    double maxfpt = 0;
    // double maxfeta = -10;
    // double maxfphi = -10;
    for(int i = 0; i < nPhoton; ++i)
    {
      if(TMath::Abs(photon_eta[i]) < offlineEtaCut)
	if(!isEle[i])
	  if(TMath::Abs(seedTime[i])<3)
	    if(swissCrx[i] < 0.9)
	      if(sigmaIetaIeta[i] > 0.002)
		if(sigmaIphiIphi[i] > 0.002)
		  if(photon_pt[i] > maxfpt) {
		    if(offlineIso){
		      if((cc4[i] + cr4[i] + ct4PtCut20[i]) < 0.9)
			if(hadronicOverEm[i] < 0.1)
			{
			  maxfpt = photon_pt[i];
			  // maxfeta = photon_eta[i];
			  // maxfphi = photon_phi[i];
			}
		    } else {
		      maxfpt = photon_pt[i];
		      // maxfeta = photon_eta[i];
		      // maxfphi = photon_phi[i];
		    }
		  }
    }

    fPt->Fill(maxfpt);
    for(int i = 0; i < NTRIG; ++i)
    {
      if(triggers[i])
      {
	accepted[i]->Fill(maxfpt);
      }
    }
  }
  std::cout << "Events matched: " << matched << std::endl;

  //make turn-on curves
  TGraphAsymmErrors *a_pt[NTRIG];
  for(int i = 0; i < NTRIG; ++i){
    a_pt[i] = new TGraphAsymmErrors();
    a_pt[i]->BayesDivide(accepted[i],fPt);
    a_pt[i]->SetName(trigname[i]+"_asymm");
  }

  //save output
  TFile *outFile = TFile::Open(outFileName,"RECREATE");
  outFile->cd();
  fPt->Write();
  for(int i = 0; i < NTRIG; ++i)
  {
    accepted[i]->Write();
    a_pt[i]->Write();
  }

  HLTFile->Close();
  inFile->Close();
  outFile->Close();
}

int main(int argc, char **argv)
{
  if(argc == 4)
  {
    matchPhotonTree(argv[1], argv[2], argv[3]);
    return 0;
  }
  return 1;
}
