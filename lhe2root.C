// File: lhe2root.C
// ROOT macro to convert an LHE file into a ROOT TTree.
// Usage (from shell):
//   root -l -q 'lhe2root.C("events.lhe","events.root", -1)'
//
// Output tree: "Events"
//  Event-level branches:
//    NUP/I, IDPRUP/I, XWGTUP/D, SCALUP/D, AQEDUP/D, AQCDUP/D, EventNumber/L
//  Per-particle branches (std::vector):
//    IDUP, ISTUP, MOTH1, MOTH2, ICOL1, ICOL2 (int)
//    PX, PY, PZ, E, M, VTIMUP, SPINUP (double)
//
// Notes:
//  - Robust to comments ('# ...') and blank lines.
//  - Ignores optional XML tags inside <event> (e.g. <rwgt>) after reading NUP particles.
//  - Mother indices are stored tels quels (convention LHE: indices 1-based).
//  - Works with LHEF v1/v2 formats with the standard 6-value event header.
//
// Explanation of each variable:
//  * <event> header line has 6 numbers:
//      NUP     : number of particle lines that follow in this event (int)
//      IDPRUP  : process ID as defined by the generator (int)
//      XWGTUP  : event weight (double), may be ± for NLO
//      SCALUP  : scale used for this event (e.g. factorization/renormalization) (double)
//      AQEDUP  : alpha_QED value used for the event (double)
//      AQCDUP  : alpha_s   value used for the event (double)
//  * Each particle line has 13 values (here parsed into):
//      IDUP    : PDG ID (int; sign is charge)
//      ISTUP   : status code (int; e.g. -1 incoming, 1 final-state, 2 intermediate resonance, etc.)
//      MOTHUP1 : index of first mother (1-based; 0 if none)
//      MOTHUP2 : index of second mother (1-based; 0 if none)
//      ICOL1   : color tag 1 (int; 0 if not colored)
//      ICOL2   : color tag 2 (int; 0 if not colored)
//      PX,PY,PZ: 3-momentum components (double) in GeV
//      E       : energy (double) in GeV
//      M       : invariant mass (double) in GeV
//      VTIMUP  : invariant lifetime (c*tau) in mm or generator units (double)
//      SPINUP  : spin info; commonly cos(theta) of spin vector relative to momentum (double)
//
// Note: When you will use MakeSelector("...") to analyze this TTree, here is how you will be able to retrieve variables in the Process() function : 
//   - For event-level scalars (e.g. NUP, XWGTUP, SCALUP), declare them as TTreeReaderValue<T> and access with *NUP, *XWGTUP, etc.
//   - For particle-level vectors (e.g. IDUP, PX, E), declare them as TTreeReaderValue<std::vector<type>> and access with (*IDUP)[i], (*PX)[i], etc.
//   - Example (inside Process):
//         fReader.SetEntry(entry);
//         int nup = *NUP;                // number of particles
//         double weight = *XWGTUP;       // event weight
//         const auto& ids = *IDUP;       // vector of PDG IDs
//         const auto& px  = *PX;         // vector of px values
//         for (size_t i=0; i<ids.size(); ++i) {
//             int pdg = ids[i];
//             double pxi = px[i];
//             // ... do your analysis ...
//         }

#include "TFile.h"
#include "TTree.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace {
  inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){return !std::isspace(ch);} ));
  }
  inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){return !std::isspace(ch);} ).base(), s.end());
  }
  inline void trim(std::string &s) { ltrim(s); rtrim(s); }

  // Remove inline comments starting with '#'
  inline void strip_comment(std::string &s) {
    auto pos = s.find('#');
    if (pos != std::string::npos) s.erase(pos);
  }

  // Return true if the (trimmed) line is an XML tag (starts with '<')
  inline bool is_xml_tag(const std::string &s) {
    for (char c : s) {
      if (!std::isspace(static_cast<unsigned char>(c))) return (c == '<');
    }
    return false;
  }
}

void lhe2root(const char* lheFile = "events.lhe",
              const char* outFile = "events.root",
              Long64_t maxEvents = -1)
{
  std::ifstream fin(lheFile);
  if (!fin.is_open()) {
    std::cerr << "[ERROR] Cannot open LHE file: " << lheFile << std::endl;
    return;
  }

  TFile *fout = TFile::Open(outFile, "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "[ERROR] Cannot create ROOT file: " << outFile << std::endl;
    return;
  }

  // Event-level variables
  Int_t    NUP = 0;
  Int_t    IDPRUP = 0;
  Double_t XWGTUP = 0.0;
  Double_t SCALUP = 0.0;
  Double_t AQEDUP = 0.0;
  Double_t AQCDUP = 0.0;
  Long64_t EventNumber = 0;

  // Particle-level vectors (use pointers for robust ROOT I/O)
  auto IDUP   = new std::vector<int>();
  auto ISTUP  = new std::vector<int>();
  auto MOTH1  = new std::vector<int>();
  auto MOTH2  = new std::vector<int>();
  auto ICOL1  = new std::vector<int>();
  auto ICOL2  = new std::vector<int>();

  auto PX     = new std::vector<double>();
  auto PY     = new std::vector<double>();
  auto PZ     = new std::vector<double>();
  auto E      = new std::vector<double>();
  auto M      = new std::vector<double>();
  auto VTIMUP = new std::vector<double>();
  auto SPINUP = new std::vector<double>();

  TTree *t = new TTree("Events", "LHE events");
  t->Branch("EventNumber", &EventNumber, "EventNumber/L");
  t->Branch("NUP",   &NUP,   "NUP/I");
  t->Branch("IDPRUP",&IDPRUP,"IDPRUP/I");
  t->Branch("XWGTUP",&XWGTUP,"XWGTUP/D");
  t->Branch("SCALUP",&SCALUP,"SCALUP/D");
  t->Branch("AQEDUP",&AQEDUP,"AQEDUP/D");
  t->Branch("AQCDUP",&AQCDUP,"AQCDUP/D");

  t->Branch("IDUP",   &IDUP);
  t->Branch("ISTUP",  &ISTUP);
  t->Branch("MOTH1",  &MOTH1);
  t->Branch("MOTH2",  &MOTH2);
  t->Branch("ICOL1",  &ICOL1);
  t->Branch("ICOL2",  &ICOL2);

  t->Branch("PX",     &PX);
  t->Branch("PY",     &PY);
  t->Branch("PZ",     &PZ);
  t->Branch("E",      &E);
  t->Branch("M",      &M);
  t->Branch("VTIMUP", &VTIMUP);
  t->Branch("SPINUP", &SPINUP);

  enum State { OUTSIDE, EXPECT_HEADER, READING_PARTICLES, SKIP_TO_END };
  State state = OUTSIDE;

  std::string line;
  int particlesRead = 0;

  auto clear_vectors = [&]() {
    IDUP->clear(); ISTUP->clear(); MOTH1->clear(); MOTH2->clear();
    ICOL1->clear(); ICOL2->clear();
    PX->clear(); PY->clear(); PZ->clear(); E->clear(); M->clear();
    VTIMUP->clear(); SPINUP->clear();
  };

  Long64_t filled = 0;
  while (std::getline(fin, line)) {
    strip_comment(line);
    trim(line);
    if (line.empty()) continue;

    if (state == OUTSIDE) {
      if (line.find("<event") != std::string::npos) {
        state = EXPECT_HEADER;
        // prepare new event
        NUP = 0; IDPRUP = 0; XWGTUP = SCALUP = AQEDUP = AQCDUP = 0.0;
        particlesRead = 0;
        clear_vectors();
      }
      continue;
    }

    if (state == EXPECT_HEADER) {
      if (is_xml_tag(line)) continue; // skip any xml lines until numeric header

      // Parse the 6-value event header line
      {
        std::istringstream hs(line);
        if (!(hs >> NUP >> IDPRUP >> XWGTUP >> SCALUP >> AQEDUP >> AQCDUP)) {
          // Not the header yet; keep looking
          continue;
        }
      }
      state = READING_PARTICLES;
      continue;
    }

    if (state == READING_PARTICLES) {
      // Stop if we encounter end of event before reading all particles (malformed)
      if (line.find("</event") != std::string::npos) {
        // Fill what we have
        EventNumber = filled;
        t->Fill();
        ++filled;
        state = OUTSIDE;
        if (maxEvents >= 0 && filled >= maxEvents) break;
        continue;
      }

      if (is_xml_tag(line)) {
        // Ignore inner XML tags (e.g., <rwgt>) while reading particles.
        continue;
      }

      // Parse one particle line (13 values standard)
      {
        int id, ist, moth1, moth2, icol1, icol2;
        double px, py, pz, e, m, vtim, spin;
        std::istringstream ps(line);
        if (ps >> id >> ist >> moth1 >> moth2 >> icol1 >> icol2
               >> px >> py >> pz >> e >> m >> vtim >> spin) {
          IDUP->push_back(id);
          ISTUP->push_back(ist);
          MOTH1->push_back(moth1);
          MOTH2->push_back(moth2);
          ICOL1->push_back(icol1);
          ICOL2->push_back(icol2);
          PX->push_back(px);
          PY->push_back(py);
          PZ->push_back(pz);
          E->push_back(e);
          M->push_back(m);
          VTIMUP->push_back(vtim);
          SPINUP->push_back(spin);
          ++particlesRead;
        } else {
          // Not a valid particle line; skip
          continue;
        }
      }

      if (particlesRead >= NUP) {
        // We have all particles; now skip until </event>
        state = SKIP_TO_END;
      }
      continue;
    }

    if (state == SKIP_TO_END) {
      if (line.find("</event") != std::string::npos) {
        EventNumber = filled;
        t->Fill();
        ++filled;
        state = OUTSIDE;
        if (maxEvents >= 0 && filled >= maxEvents) break;
      }
      // ignore anything else inside the event
      continue;
    }
  }

  fout->cd();
  t->Write();
  fout->Close();

  std::cout << "[INFO] Converted " << filled << " events from " << lheFile
            << " into " << outFile << std::endl;
}

// Convenience wrapper to match ROOT's -q call without args
void lhe2root() { lhe2root("events.lhe", "events.root", -1); }
