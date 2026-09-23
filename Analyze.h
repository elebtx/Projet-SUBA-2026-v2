//////////////////////////////////////////////////////////
// Analyze.h
// Classe TSelector utilisee par Analyze.C
// ROOT 6.40.04 - TTree "Events"
//////////////////////////////////////////////////////////

#ifndef Analyze_h
#define Analyze_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TSelector.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <TH1D.h>
#include <TLorentzVector.h>
#include <TF1.h>
#include <TMath.h>
#include <cmath>
#include <TEfficiency.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TLine.h>
#include <TLegend.h>
#include <vector>
#include <TFitResult.h>
#include <TFitResultPtr.h>

class Analyze : public TSelector {
public:
   TTreeReader fReader;
   TTree *fChain = 0;

   TTreeReaderValue<Long64_t> EventNumber = {fReader, "EventNumber"};
   TTreeReaderValue<Int_t> NUP = {fReader, "NUP"};
   TTreeReaderValue<Int_t> IDPRUP = {fReader, "IDPRUP"};
   TTreeReaderValue<Double_t> XWGTUP = {fReader, "XWGTUP"};
   TTreeReaderValue<Double_t> SCALUP = {fReader, "SCALUP"};
   TTreeReaderValue<Double_t> AQEDUP = {fReader, "AQEDUP"};
   TTreeReaderValue<Double_t> AQCDUP = {fReader, "AQCDUP"};

   TTreeReaderArray<int> IDUP = {fReader, "IDUP"};
   TTreeReaderArray<int> ISTUP = {fReader, "ISTUP"};
   TTreeReaderArray<int> MOTH1 = {fReader, "MOTH1"};
   TTreeReaderArray<int> MOTH2 = {fReader, "MOTH2"};
   TTreeReaderArray<int> ICOL1 = {fReader, "ICOL1"};
   TTreeReaderArray<int> ICOL2 = {fReader, "ICOL2"};
   TTreeReaderArray<double> PX = {fReader, "PX"};
   TTreeReaderArray<double> PY = {fReader, "PY"};
   TTreeReaderArray<double> PZ = {fReader, "PZ"};
   TTreeReaderArray<double> E = {fReader, "E"};
   TTreeReaderArray<double> M = {fReader, "M"};
   TTreeReaderArray<double> VTIMUP = {fReader, "VTIMUP"};
   TTreeReaderArray<double> SPINUP = {fReader, "SPINUP"};

   TH1D *h_massZ = nullptr;
   TH1D *h_ptLead_total = nullptr;
   TH1D *h_ptLead_pass = nullptr;
   TH1D *h_etaMuon = nullptr;
   TH1D *h_eta_tot= nullptr;
   TH1D *h_eta_pass = nullptr;


   Analyze(TTree * = 0) {}
   ~Analyze() override {}
   Int_t Version() const override { return 2; }

   void Begin(TTree *tree) override;
   void SlaveBegin(TTree *tree) override;
   void Init(TTree *tree) override;
   bool Notify() override;
   bool Process(Long64_t entry) override;
   Int_t GetEntry(Long64_t entry, Int_t getall = 0) override {
      return fChain ? fChain->GetTree()->GetEntry(entry, getall) : 0;
   }
   void SetOption(const char *option) override { fOption = option; }
   void SetObject(TObject *obj) override { fObject = obj; }
   void SetInputList(TList *input) override { fInput = input; }
   TList *GetOutputList() const override { return fOutput; }
   void SlaveTerminate() override;
   void Terminate() override;

   ClassDefOverride(Analyze,0);
};

#ifdef Analyze_cxx
void Analyze::Init(TTree *tree)
{
   fReader.SetTree(tree);
   fChain = tree;
}

bool Analyze::Notify()
{
   return true;
}

#endif

#endif
