// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "PWGDQ/Core/VarManager.h"

#include <cmath>

ClassImp(VarManager);

TString VarManager::fgVariableNames[VarManager::kNVars] = {""};
TString VarManager::fgVariableUnits[VarManager::kNVars] = {""};
bool VarManager::fgUsedVars[VarManager::kNVars] = {kFALSE};
float VarManager::fgValues[VarManager::kNVars] = {0.0f};
std::map<int, int> VarManager::fgRunMap;
TString VarManager::fgRunStr = "";
o2::vertexing::DCAFitterN<2> VarManager::fgFitterTwoProng;
o2::vertexing::FwdDCAFitterN<2> VarManager::FwdfgFitterTwoProng;

//__________________________________________________________________
VarManager::VarManager() : TObject()
{
  //
  // constructor
  //
  SetDefaultVarNames();
}

//__________________________________________________________________
VarManager::~VarManager() = default;

//__________________________________________________________________
void VarManager::SetVariableDependencies()
{
  //
  // Set as used variables on which other variables calculation depends
  //
  if (fgUsedVars[kP]) {
    fgUsedVars[kPt] = kTRUE;
    fgUsedVars[kEta] = kTRUE;
  }
}

//__________________________________________________________________
void VarManager::ResetValues(int startValue, int endValue, float* values)
{
  //
  // reset all variables to an "innocent" value
  // NOTE: here we use -9999.0 as a neutral value, but depending on situation, this may not be the case
  if (!values) {
    values = fgValues;
  }
  for (Int_t i = startValue; i < endValue; ++i) {
    values[i] = -9999.;
  }
}

//__________________________________________________________________
void VarManager::SetRunNumbers(int n, int* runs)
{
  //
  // maps the list of runs such that one can plot the list of runs nicely in a histogram axis
  //
  for (int i = 0; i < n; ++i) {
    fgRunMap[runs[i]] = i + 1;
    fgRunStr += Form("%d;", runs[i]);
  }
}

//__________________________________________________________________
void VarManager::SetRunNumbers(std::vector<int> runs)
{
  //
  // maps the list of runs such that one can plot the list of runs nicely in a histogram axis
  //
  int i = 0;
  for (auto run = runs.begin(); run != runs.end(); run++, i++) {
    fgRunMap[*run] = i + 1;
    fgRunStr += Form("%d;", *run);
  }
}

//__________________________________________________________________
void VarManager::FillEventDerived(float* values)
{
  //
  // Fill event-wise derived quantities (these are all quantities which can be computed just based on the values already filled in the FillEvent() function)
  //
  if (fgUsedVars[kRunId]) {
    values[kRunId] = (fgRunMap.size() > 0 ? fgRunMap[int(values[kRunNo])] : 0);
  }
}

//__________________________________________________________________
void VarManager::FillTrackDerived(float* values)
{
  //
  // Fill track-wise derived quantities (these are all quantities which can be computed just based on the values already filled in the FillTrack() function)
  //
  if (fgUsedVars[kP]) {
    values[kP] = values[kPt] * std::cosh(values[kEta]);
  }
}

//__________________________________________________________________
void VarManager::SetDefaultVarNames()
{
  //
  // Set default variable names
  //
  for (Int_t ivar = 0; ivar < kNVars; ++ivar) {
    fgVariableNames[ivar] = "DEFAULT NOT DEFINED";
    fgVariableUnits[ivar] = "n/a";
  }

  fgVariableNames[kRunNo] = "Run number";
  fgVariableUnits[kRunNo] = "";
  fgVariableNames[kRunId] = "Run number";
  fgVariableUnits[kRunId] = "";
  fgVariableNames[kBC] = "Bunch crossing";
  fgVariableUnits[kBC] = "";
  fgVariableNames[kIsPhysicsSelection] = "Physics selection";
  fgVariableUnits[kIsPhysicsSelection] = "";
  fgVariableNames[kVtxX] = "Vtx X ";
  fgVariableUnits[kVtxX] = "cm";
  fgVariableNames[kVtxY] = "Vtx Y ";
  fgVariableUnits[kVtxY] = "cm";
  fgVariableNames[kVtxZ] = "Vtx Z ";
  fgVariableUnits[kVtxZ] = "cm";
  fgVariableNames[kVtxNcontrib] = "Vtx contrib.";
  fgVariableUnits[kVtxNcontrib] = "";
  fgVariableNames[kVtxCovXX] = "Vtx covXX";
  fgVariableUnits[kVtxCovXX] = "cm";
  fgVariableNames[kVtxCovXY] = "Vtx covXY";
  fgVariableUnits[kVtxCovXY] = "cm";
  fgVariableNames[kVtxCovXZ] = "Vtx covXZ";
  fgVariableUnits[kVtxCovXZ] = "cm";
  fgVariableNames[kVtxCovYY] = "Vtx covYY";
  fgVariableUnits[kVtxCovYY] = "cm";
  fgVariableNames[kVtxCovYZ] = "Vtx covYZ";
  fgVariableUnits[kVtxCovYZ] = "cm";
  fgVariableNames[kVtxCovZZ] = "Vtx covZZ";
  fgVariableUnits[kVtxCovZZ] = "cm";
  fgVariableNames[kVtxChi2] = "Vtx chi2";
  fgVariableUnits[kVtxChi2] = "";
  fgVariableNames[kCentVZERO] = "Centrality VZERO";
  fgVariableUnits[kCentVZERO] = "%";
  fgVariableNames[kMCEventGeneratorId] = "MC Generator ID";
  fgVariableNames[kMCVtxX] = "MC Vtx X";
  fgVariableNames[kMCVtxY] = "MC Vtx Y";
  fgVariableNames[kMCVtxZ] = "MC Vtx Z";
  fgVariableNames[kMCEventTime] = "MC event time";
  fgVariableNames[kMCEventWeight] = "MC event weight";
  fgVariableNames[kMCEventImpParam] = "MC impact parameter";
  fgVariableUnits[kMCEventGeneratorId] = "";
  fgVariableUnits[kMCVtxX] = "cm";
  fgVariableUnits[kMCVtxY] = "cm";
  fgVariableUnits[kMCVtxZ] = "cm";
  fgVariableUnits[kMCEventTime] = ""; // TODO: add proper unit
  fgVariableUnits[kMCEventWeight] = "";
  fgVariableUnits[kMCEventImpParam] = "b";
  fgVariableNames[kPt] = "p_{T}";
  fgVariableUnits[kPt] = "GeV/c";
  fgVariableNames[kP] = "p";
  fgVariableUnits[kP] = "GeV/c";
  fgVariableNames[kPx] = "p_{x}";
  fgVariableUnits[kPy] = "GeV/c";
  fgVariableNames[kPy] = "p_{y}";
  fgVariableUnits[kPz] = "GeV/c";
  fgVariableNames[kPz] = "p_{z}";
  fgVariableUnits[kPx] = "GeV/c";
  fgVariableNames[kEta] = "#eta";
  fgVariableUnits[kEta] = "";
  fgVariableNames[kPhi] = "#varphi";
  fgVariableUnits[kPhi] = "rad.";
  fgVariableNames[kRap] = "y";
  fgVariableUnits[kRap] = "";
  fgVariableNames[kMass] = "mass";
  fgVariableUnits[kMass] = "GeV/c2";
  fgVariableNames[kCharge] = "charge";
  fgVariableUnits[kCharge] = "";
  fgVariableNames[kPin] = "p_{IN}";
  fgVariableUnits[kPin] = "GeV/c";
  fgVariableNames[kITSncls] = "ITS #cls";
  fgVariableUnits[kITSncls] = "";
  fgVariableNames[kITSchi2] = "ITS chi2";
  fgVariableUnits[kITSchi2] = "";
  fgVariableNames[kITSlayerHit] = "ITS layer";
  fgVariableUnits[kITSlayerHit] = "";
  fgVariableNames[kTPCncls] = "TPC #cls";
  fgVariableUnits[kTPCncls] = "";
  fgVariableNames[kTPCnclsCR] = "TPC #cls crossed rows";
  fgVariableUnits[kTPCnclsCR] = "";
  fgVariableNames[kTPCchi2] = "TPC chi2";
  fgVariableUnits[kTPCchi2] = "";
  fgVariableNames[kTPCsignal] = "TPC dE/dx";
  fgVariableUnits[kTPCsignal] = "";
  fgVariableNames[kTRDsignal] = "TRD dE/dx";
  fgVariableUnits[kTRDsignal] = "";
  fgVariableNames[kTOFbeta] = "TOF #beta";
  fgVariableUnits[kTOFbeta] = "";
  fgVariableNames[kTrackLength] = "track length";
  fgVariableUnits[kTrackLength] = "cm";
  fgVariableNames[kTrackDCAxy] = "DCA_{xy}";
  fgVariableUnits[kTrackDCAxy] = "cm";
  fgVariableNames[kTrackDCAz] = "DCA_{z}";
  fgVariableUnits[kTrackDCAz] = "cm";
  fgVariableNames[kTPCnSigmaEl] = "n #sigma_{e}^{TPC}";
  fgVariableUnits[kTPCnSigmaEl] = "";
  fgVariableNames[kTPCnSigmaMu] = "n #sigma_{#mu}^{TPC}";
  fgVariableUnits[kTPCnSigmaMu] = "";
  fgVariableNames[kTPCnSigmaPi] = "n #sigma_{#pi}^{TPC}";
  fgVariableUnits[kTPCnSigmaPi] = "";
  fgVariableNames[kTPCnSigmaKa] = "n #sigma_{K}^{TPC}";
  fgVariableUnits[kTPCnSigmaKa] = "";
  fgVariableNames[kTPCnSigmaPr] = "n #sigma_{p}^{TPC}";
  fgVariableUnits[kTPCnSigmaPr] = "";
  fgVariableNames[kTOFnSigmaEl] = "n #sigma_{e}^{TOF}";
  fgVariableUnits[kTOFnSigmaEl] = "";
  fgVariableNames[kTOFnSigmaMu] = "n #sigma_{#mu}^{TOF}";
  fgVariableUnits[kTOFnSigmaMu] = "";
  fgVariableNames[kTOFnSigmaPi] = "n #sigma_{#pi}^{TOF}";
  fgVariableUnits[kTOFnSigmaPi] = "";
  fgVariableNames[kTOFnSigmaKa] = "n #sigma_{K}^{TOF}";
  fgVariableUnits[kTOFnSigmaKa] = "";
  fgVariableNames[kTOFnSigmaPr] = "n #sigma_{p}^{TOF}";
  fgVariableUnits[kTOFnSigmaPr] = "";
  fgVariableNames[kIsLegFromGamma] = "is leg from #gamma #rightarror e^{+}e^{-}";
  fgVariableUnits[kIsLegFromGamma] = "";
  fgVariableNames[kIsLegFromK0S] = "is leg from K_{S}^{0} #rightarror #pi^{+}#pi^{-}";
  fgVariableUnits[kIsLegFromK0S] = "";
  fgVariableNames[kIsLegFromLambda] = "is leg from #Lambda #rightarror p#pi^{-}";
  fgVariableUnits[kIsLegFromLambda] = "";
  fgVariableNames[kIsLegFromAntiLambda] = "is leg from #bar{#Lambda} #rightarrow #bar{p}#pi^{+}";
  fgVariableUnits[kIsLegFromAntiLambda] = "";
  fgVariableNames[kIsLegFromOmega] = "is leg from #Omega^{#mp} #rightarrow #LambdaKi^{#pm}";
  fgVariableUnits[kIsLegFromOmega] = "";
  fgVariableNames[kMuonNClusters] = "muon n-clusters";
  fgVariableUnits[kMuonNClusters] = "";
  fgVariableNames[kMuonRAtAbsorberEnd] = "R at the end of the absorber";
  fgVariableUnits[kMuonRAtAbsorberEnd] = "cm";
  fgVariableNames[kMuonPDca] = "p x dca";
  fgVariableUnits[kMuonPDca] = "cm x GeV/c";
  fgVariableNames[kMCHBitMap] = "MCH bitmap";
  fgVariableUnits[kMCHBitMap] = "";
  fgVariableNames[kMuonChi2] = "#chi^{2}";
  fgVariableUnits[kMuonChi2] = "";
  fgVariableNames[kMuonChi2MatchMCHMID] = "#chi^{2} MCH-MID";
  fgVariableUnits[kMuonChi2MatchMCHMID] = "";
  fgVariableNames[kMuonChi2MatchMCHMFT] = "#chi^{2} MCH-MFT";
  fgVariableUnits[kMuonChi2MatchMCHMFT] = "";
  fgVariableNames[kMuonMatchScoreMCHMFT] = "match score MCH-MFT";
  fgVariableUnits[kMuonMatchScoreMCHMFT] = "";
  fgVariableNames[kMuonCXX] = "cov XX";
  fgVariableUnits[kMuonCXX] = "";
  fgVariableNames[kMuonCYY] = "cov YY";
  fgVariableUnits[kMuonCYY] = "";
  fgVariableNames[kMuonCPhiPhi] = "cov PhiPhi";
  fgVariableUnits[kMuonCPhiPhi] = "";
  fgVariableNames[kMuonCTglTgl] = "cov TglTgl";
  fgVariableUnits[kMuonCTglTgl] = "";
  fgVariableNames[kMuonC1Pt21Pt2] = "cov 1Pt1Pt";
  fgVariableUnits[kMuonC1Pt21Pt2] = "";
  fgVariableNames[kMCPdgCode] = "MC PDG code";
  fgVariableUnits[kMCPdgCode] = "";
  fgVariableNames[kMCParticleWeight] = "MC particle weight";
  fgVariableUnits[kMCParticleWeight] = "";
  fgVariableNames[kMCPx] = "MC px";
  fgVariableUnits[kMCPx] = "GeV/c";
  fgVariableNames[kMCPy] = "MC py";
  fgVariableUnits[kMCPy] = "GeV/c";
  fgVariableNames[kMCPz] = "MC pz";
  fgVariableUnits[kMCPz] = "GeV/c";
  fgVariableNames[kMCPt] = "MC pt";
  fgVariableUnits[kMCPt] = "GeV/c";
  fgVariableNames[kMCPhi] = "#varphi";
  fgVariableUnits[kMCPhi] = "rad";
  fgVariableNames[kMCEta] = "MC #eta";
  fgVariableUnits[kMCEta] = "";
  fgVariableNames[kMCY] = "MC y";
  fgVariableUnits[kMCY] = "";
  fgVariableNames[kMCE] = "MC Energy";
  fgVariableUnits[kMCE] = "GeV";
  fgVariableNames[kMCVx] = "MC vx";
  fgVariableUnits[kMCVx] = "cm"; // TODO: check the unit
  fgVariableNames[kMCVy] = "MC vy";
  fgVariableUnits[kMCVy] = "cm"; // TODO: check the unit
  fgVariableNames[kMCVz] = "MC vz";
  fgVariableUnits[kMCVz] = "cm"; // TODO: check the unit
  fgVariableNames[kCandidateId] = "";
  fgVariableUnits[kCandidateId] = "";
  fgVariableNames[kPairType] = "Pair type";
  fgVariableUnits[kPairType] = "";
  fgVariableNames[kVertexingLxy] = "Pair Lxy";
  fgVariableUnits[kVertexingLxy] = "cm";
  fgVariableNames[kVertexingLz] = "Pair Lz";
  fgVariableUnits[kVertexingLz] = "cm";
  fgVariableNames[kVertexingLxyz] = "Pair Lxyz";
  fgVariableUnits[kVertexingLxyz] = "cm";
  fgVariableNames[kVertexingLxyErr] = "Pair Lxy err.";
  fgVariableUnits[kVertexingLxyErr] = "cm";
  fgVariableNames[kVertexingLzErr] = "Pair Lz err.";
  fgVariableUnits[kVertexingLzErr] = "cm";
  fgVariableNames[kVertexingLxyzErr] = "Pair Lxyz err.";
  fgVariableUnits[kVertexingLxyzErr] = "cm";
  fgVariableNames[kVertexingTauz] = "Pair pseudo-proper Tauz";
  fgVariableUnits[kVertexingTauz] = "ns";
  fgVariableNames[kVertexingTauxy] = "Pair pseudo-proper Tauxy";
  fgVariableUnits[kVertexingTauxy] = "ns";
  fgVariableNames[kVertexingTauzErr] = "Pair pseudo-proper Tauz err.";
  fgVariableUnits[kVertexingTauzErr] = "ns";
  fgVariableNames[kVertexingTauxyErr] = "Pair pseudo-proper Tauxy err.";
  fgVariableUnits[kVertexingTauxyErr] = "ns";
  fgVariableNames[kVertexingProcCode] = "DCAFitterN<2> processing code";
  fgVariableUnits[kVertexingProcCode] = "";
  fgVariableNames[kVertexingChi2PCA] = "Pair #chi^{2} at PCA";
  fgVariableUnits[kVertexingChi2PCA] = "";
  fgVariableNames[kPairMass] = "mass";
  fgVariableUnits[kPairMass] = "GeV/c2";
  fgVariableNames[kPairPt] = "p_{T}";
  fgVariableUnits[kPairPt] = "GeV/c";
  fgVariableNames[kPairEta] = "#eta";
  fgVariableUnits[kPairEta] = "";
  fgVariableNames[kPairPhi] = "#varphi";
  fgVariableUnits[kPairPhi] = "rad.";
  fgVariableNames[kDeltaEta] = "#Delta#eta";
  fgVariableUnits[kDeltaEta] = "";
  fgVariableNames[kDeltaPhi] = "#Delta#phi";
  fgVariableUnits[kDeltaPhi] = "rad.";
  fgVariableNames[kDeltaPhiSym] = "#Delta#phi";
  fgVariableUnits[kDeltaPhiSym] = "rad.";
  fgVariableNames[kCosThetaHE] = "cos#it{#theta}";
  fgVariableUnits[kCosThetaHE] = "";
  fgVariableNames[kQuadDCAabsXY] = "DCA_{xy}^{quad}";
  fgVariableUnits[kQuadDCAabsXY] = "cm";
  fgVariableNames[kQuadDCAsigXY] = "DCA_{xy}^{quad}";
  fgVariableUnits[kQuadDCAsigXY] = "#sigma";
  fgVariableNames[kTrackDCAsigXY] = "DCA_{xy}";
  fgVariableUnits[kTrackDCAsigXY] = "#sigma";
  fgVariableNames[kTrackDCAsigZ] = "DCA_{z}";
  fgVariableUnits[kTrackDCAsigZ] = "#sigma";
  fgVariableNames[kTrackDCAresXY] = "#Delta DCA_{xy}";
  fgVariableUnits[kTrackDCAresXY] = "cm";
  fgVariableNames[kTrackDCAresZ] = "#Delta DCA_{z}";
  fgVariableUnits[kTrackDCAresZ] = "cm";
  fgVariableNames[kBitMapIndex] = " ";
  fgVariableUnits[kBitMapIndex] = "";
}
