#define Analyze_cxx
#include "Analyze.h"
#include <TStyle.h>
#include <iostream>
#include <string>

// Variables globales pour le calcul de l'efficacité
Long64_t N_tot = 0;
Long64_t nPreselected = 0;
Long64_t nSelected = 0;


void Analyze::Begin(TTree *) {
   TString option = GetOption();
}

void Analyze::SlaveBegin(TTree *) {
   
   TString option = GetOption();

   // ***Création des histogrammes*** 
   // 60 bins entre 60 et 120 GeV pour bien encadrer le pic (situé vers 91 GeV)
   h_massZ = new TH1D("h_massZ", "Masse invariante;m_{#mu#mu} [GeV];Nombre d'evenements", 60, 60.0, 120.0);

   h_ptLead_total = new TH1D("h_ptLead_total",
      "pT leading muon - preselection;p_{T}^{lead} [GeV];Evenements",
      20, 0, 100);

   h_ptLead_pass = new TH1D("h_ptLead_pass",
      "pT leading muon - selection pT > 25 GeV;p_{T}^{lead} [GeV];Evenements",
      20, 0, 100);


   // Histogrammes pseudo-rapidité des muons (30 bins de largeur 0.2 entre -3.0 et 3.0)
   h_etaMuon = new TH1D("h_etaMuon","Pseudorapidite des muons;#eta_{#mu};Muons",30, -3.0, 3.0);
   h_eta_tot = new TH1D("h_eta_tot","Muons preselectionnes;#eta;Muons",30, -3.0, 3.0);
   h_eta_pass = new TH1D("h_eta_pass","Muons passant la selection;#eta;Muons",30, -3.0, 3.0);

   // Sauvegarde globale des histogrammes
   fOutput->Add(h_massZ);
   fOutput->Add(h_ptLead_total);
   fOutput->Add(h_ptLead_pass);
   fOutput->Add(h_etaMuon);
   fOutput->Add(h_eta_tot);
   fOutput->Add(h_eta_pass);

   // Permet le calcul des incertitudes
   h_massZ->Sumw2();
   h_ptLead_total->Sumw2();
   h_ptLead_pass->Sumw2();
   h_etaMuon->Sumw2();
   h_eta_tot->Sumw2();
   h_eta_pass->Sumw2();
   
}

bool Analyze::Process(Long64_t entry) {
   
   fReader.SetLocalEntry(entry);
   
   // Incrémentation du dénominateur pour l'efficacité globale
   N_tot++; 

   // Utilisation d'un vecteur pour stocker les muons
   std::vector<TLorentzVector> muons_event; 

   // Boucle sur les particules de l'événement
   for (size_t i = 0; i < IDUP.GetSize(); ++i) {
      
      // Code PID 13 = muon. ISTUP == 1 = état final
      if (std::abs(IDUP[i]) == 13 && ISTUP[i] == 1) {
         TLorentzVector mu;  
         mu.SetPxPyPzE(PX[i], PY[i], PZ[i], E[i]);
         muons_event.push_back(mu);
      }
   }

   // Pré-sélection : évènements avec exactement deux muons dans l'état final
   if (muons_event.size() != 2) return true;
   
   ++nPreselected;


   bool pass25 = false;
   double ptLead = 0.0;

   // Boucle sur les muons retenus
   for (size_t i = 0; i < muons_event.size(); ++i) {
      
      const double pt = muons_event[i].Pt();
      const double eta = muons_event[i].Eta(); 

      if (pt <= 0.0) continue;

      // Recherche du muon le plus énergétique (Leading Muon)
      if (pt > ptLead) ptLead = pt;

      // Remplissage des histogrammes au dénominateur
      h_etaMuon->Fill(eta);
      h_eta_tot->Fill(eta);

      // Sélections cinématiques :
      if (pt > 25.0 && std::abs(eta) < 2.4) {
         pass25 = true;
         h_eta_pass->Fill(eta);
      }

   }

   // Remplissage des histogrammes 
   h_ptLead_total->Fill(ptLead);

   if (pass25) {
      ++nSelected; 
      h_ptLead_pass->Fill(ptLead);

      // Calcul de la masse invariante 
      TLorentzVector Z = muons_event[0] + muons_event[1];
      h_massZ->Fill(Z.M());
   }

   return true;
}

void Analyze::SlaveTerminate() {
}

void Analyze::Terminate() {  

   // Evite les divisions par 0
   if (N_tot == 0) return;

   // Calcul de l'efficacité globale
   const double eff = double(nSelected) / N_tot;
   
   // Incertitude 1 : Approximation binomiale 
   const double err_binom = std::sqrt(eff * (1.0 - eff) / N_tot);
   
   // Incertitude 2 : Méthode de Clopper-Pearson (intervalle à 1 sigma)
   const double cp_low = TEfficiency::ClopperPearson(N_tot, nSelected, 0.683, false);
   const double cp_up = TEfficiency::ClopperPearson(N_tot, nSelected, 0.683, true);

   std::cout << "\n=== RESULTATS DE LA SELECTION ===" << std::endl;
   std::cout << "Evenements totaux : " << N_tot << std::endl;
   std::cout << "Evenements preselectionnes : " << nPreselected << std::endl;
   std::cout << "Evenements selectionnes  : " << nSelected << std::endl;
   std::cout << "Efficacite globale : " << eff << std::endl;
   std::cout << "Incertitude binomiale : " << err_binom << std::endl;
   std::cout << "Incertitude Clopper-Pearson : [" << cp_low << ", " << cp_up << "]" << std::endl;   


   gStyle->SetOptStat(0);
   //gStyle->SetOptFit(1111);

   // On récupère les histogrammes
   TH1D *hTotal = (TH1D*)GetOutputList()->FindObject("h_ptLead_total");
   TH1D *hPass = (TH1D*)GetOutputList()->FindObject("h_ptLead_pass");
   TH1D *hmassZ = (TH1D*)GetOutputList()->FindObject("h_massZ");
   TH1D *hEta = (TH1D*)GetOutputList()->FindObject("h_etaMuon");
   TH1D *hEtaTotal = (TH1D*)GetOutputList()->FindObject("h_eta_tot");
   TH1D *hEtaPass = (TH1D*)GetOutputList()->FindObject("h_eta_pass");

   if (!hTotal || !hPass || !hmassZ || !hEta || !hEtaTotal || !hEtaPass) return;


   // === TRACÉ ET FIT DE L'EFFICACITÉ EN FONCTION DE ETA ===
   TCanvas *c1 = new TCanvas("c1", "Efficacite Differentielle", 800, 600);
   c1->SetGrid(); 

   // l'option "cp" indique de calculer l'incertitude de Clopper-Pearson
   TGraphAsymmErrors *gr_eff = new TGraphAsymmErrors(hEtaPass, hEtaTotal, "cp");
   
   // Esthétique du graphique
   gr_eff->SetTitle("Efficacite de la selection en fonction de #eta;#eta ;Efficacite #epsilon");
   gr_eff->SetMarkerStyle(20);        
   gr_eff->SetMarkerColor(kBlue+1);
   gr_eff->SetLineColor(kBlue+1);
   
   // Dessiner les axes (A) et les points avec barres d'erreur (P)
   gr_eff->Draw("AP");

   
   // Choix de la fonction "pol2" (p0 + p1*x + p2*x^2) pour le fit de l'efficacité vs eta
   TF1 *fit_eff = new TF1("fit_eff", "pol2", -2.4, 2.4); 
   fit_eff->SetLineColor(kRed);
   
   std::cout << "\n--- Fit de l'efficacite ---" << std::endl;
  
   TFitResultPtr r_eff = gr_eff->Fit("fit_eff", "R S");  // L'option "R" limite le fit à l'intervall défini ci-dessus et l'option "S" sauvegarde les résultats pour récupérer le Chi2

   // Évaluation du fit 
   if (r_eff == 0) {
       double chi2_eff = r_eff->Chi2();
       double ndf_eff = r_eff->Ndf();
       std::cout << "Qualite du fit (Chi2/ndf) : " << chi2_eff/ndf_eff << std::endl;
   }
   

   c1->SaveAs("Eff_vs_Eta.png");



  // === TRACÉ ET FIT DE L'EFFICACITÉ EN FONCTION DE PT ===
   TCanvas *c2 = new TCanvas("c2", "Fit de l efficacite differentielle", 800, 600);
   c2->SetGrid();

   TGraphAsymmErrors *gEff = new TGraphAsymmErrors(hPass, hTotal, "cp");
   
   gEff->Fit(fStep, "R");

   
   gEff->SetTitle("Fit de l'efficacite de selection;p_{T}^{lead} [GeV];Efficacite");
   gEff->SetMarkerStyle(20);
   gEff->SetMarkerColor(kBlue+1);
   gEff->SetLineColor(kBlue+1);
   gEff->SetMinimum(0.0);
   gEff->SetMaximum(1.05);
   gEff->Draw("AP");

   // Choix de la fonction "step" pour le fit de l'efficacité vs pT
   TF1 *fStep = new TF1("fStep", "x < 25.0 ? 0.0 : [0]", 10.0, 100.0);
   fStep->SetParName(0, "Plateau");
   fStep->SetParameter(0, 1.0);
   fStep->SetLineColor(kRed+1);
   fStep->Draw("SAME");

   c3->SaveAs("Eff_vs_pT.png");



   // === TRACÉ ET FIT DE LA MASSE INVARIANTE ===
   TCanvas *c3 = new TCanvas("c3", "Masse Invariante Z", 800, 600);
   c3->SetGrid();
   
   // Définition de la fonction Breit-Wigner Relativiste
   TF1 *bw = new TF1("bw", "[0] * ([1]*[1] * [2]*[2]) / ( (x*x - [1]*[1])*(x*x - [1]*[1]) + [1]*[1]*[2]*[2] )", 70.0, 110.0);
   bw->SetParameters(5000, 90, 1.5); // Initialisation: Norm, m_Z, Gamma_Z
   bw->SetParNames("Norm", "m_Z", "#Gamma_Z");
   bw->SetLineColor(kRed);

   std::cout << "\n--- Fit de la masse invariante (Breit-Wigner) ---" << std::endl;
   // Lancement de du fit réel : modification des paramètres pour que le fit corresponde parfaitement aux données 
   TFitResultPtr r = hmassZ->Fit("bw", "S"); // L'option "S" sauvegarde et renvoi les résultats du fit dans la variable r
   
   hmassZ->SetTitle("Masse invariante des deux muons;m_{#mu#mu} [GeV];Nombre d'evenements");
   hmassZ->SetMarkerStyle(20);
   hmassZ->SetLineColor(kBlack);
   hmassZ->Draw("E"); 
   
   c3->SaveAs("Fit_Masse_Z.png");

   // Évaluation du fit 
   if (r == 0) {
       double chi2 = r->Chi2();
       double ndf = r->Ndf();
       std::cout << "\nQualite du fit (Chi2/ndf) : " << chi2 << " / " << ndf << " = " << chi2/ndf << std::endl;
   }


}
