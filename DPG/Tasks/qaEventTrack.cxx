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

///
/// \file   qaEventTrack.cxx
/// \author Peter Hristov <Peter.Hristov@cern.ch>, CERN
/// \author Gian Michele Innocenti <gian.michele.innocenti@cern.ch>, CERN
/// \author Henrique J C Zanoli <henrique.zanoli@cern.ch>, Utrecht University
/// \author Mario Krüger <mario.kruger@cern.ch>
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>, CERN
/// \brief  Task to produce QA objects for the track and the event properties in the AOD.
///         This task can also be configured to produce a table with reduced information used for correlation studies for track selection
///

#include "qaEventTrack.h"

#include "Framework/AnalysisTask.h"
#include "Framework/HistogramRegistry.h"
#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisDataModel.h"
#include "ReconstructionDataFormats/DCA.h"
#include "Common/Core/trackUtilities.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/Core/TrackSelection.h"
#include "Common/Core/TrackSelectionDefaults.h"
#include "Common/TableProducer/PID/pidTOFBase.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::dataformats;

// TODO: add PID wagons as dependency + include impact parameter studies (same or separate task in workflow??)

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// Task declaration
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
struct qaEventTrack {
  // Tables to produce
  Produces<o2::aod::DPGCollisions> tableCollisions;
  Produces<o2::aod::DPGTracks> tableTracks;
  Produces<o2::aod::DPGRecoParticles> tableRecoParticles;
  Produces<o2::aod::DPGNonRecoParticles> tableNonRecoParticles;

  // general steering settings
  Configurable<bool> isRun3{"isRun3", false, "Is Run3 dataset"}; // TODO: derive this from metadata once possible to get rid of the flag

  // options to select specific events
  Configurable<bool> selectGoodEvents{"selectGoodEvents", true, "select good events"};
  // selection specific to the table creation workflow
  Configurable<float> selectMaxVtxZ{"selectMaxVtxZ", 100.f, "Derived data option: select collision in a given Z window"};
  Configurable<int> targetNumberOfEvents{"targetNumberOfEvents", 10000000, "Derived data option: target number of collisions, if the target is met, future collisions will be skipped"};
  Configurable<float> fractionOfSampledEvents{"fractionOfSampledEvents", 1.f, "Derived data option: fraction of events to sample"};

  // options to select only specific tracks
  Configurable<bool> selectGlobalTracks{"selectGlobalTracks", true, "select global tracks"};
  Configurable<bool> debugFilterkAcceptanceTrkSel{"debugFilterkAcceptanceTrkSel", false, "debug filter: select acceptance tracks (track selection workflow)"};
  Configurable<bool> debugFilterkAcceptaceManualPtMin{"debugFilterkAcceptaceManualPtMin", false, "debug filter: select acceptance tracks (manual, pt min only)"};
  Configurable<bool> debugFilterkAcceptaceManualPtMinMax{"debugFilterkAcceptaceManualPtMinMax", false, "debug filter: select acceptance tracks (manual, pt min and max)"};
  Configurable<int> selectCharge{"selectCharge", 0, "select charge +1 or -1 (0 means no selection)"};
  Configurable<bool> selectPrim{"selectPrim", false, "select primaries"};
  Configurable<bool> selectSec{"selectSec", false, "select secondaries"};
  Configurable<int> selectPID{"selectPID", 0, "select pid"};

  // configurable binning of histograms
  ConfigurableAxis binsPt{"binsPt", {VARIABLE_WIDTH, 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 2.0, 5.0, 10.0, 20.0, 50.0}, ""};

  ConfigurableAxis binsVertexPosZ{"binsVertexPosZ", {100, -20., 20.}, ""}; // TODO: do we need this to be configurable?
  ConfigurableAxis binsVertexPosXY{"binsVertexPosXY", {500, -1., 1.}, ""}; // TODO: do we need this to be configurable?
  ConfigurableAxis binsTrackMultiplicity{"binsTrackMultiplcity", {200, 0, 200}, ""};

  // TODO: ask if one can have different filters for both process functions
  Filter trackFilter = (selectGlobalTracks.node() == false) || requireGlobalTrackInFilter(); // default: lobal tracks
  // Partition<aod::TracksIU> tracksIUFiltered = requireTrackCutInFilter(TrackSelectionFlags::kInAcceptanceTracks); // this cannot work because TrackSelection only on tracks and not tracksIU
  // debug filters - acceptance
  Filter trackFilterPtEtaTrkSel = (debugFilterkAcceptanceTrkSel.node() == false) || requireTrackCutInFilter(TrackSelectionFlags::kInAcceptanceTracks);
  Filter trackFilterPtEtaManualPtMin = (debugFilterkAcceptaceManualPtMin.node() == false) || ((aod::track::pt > 0.1f) && (nabs(aod::track::eta) < 0.8f));
  Filter trackFilterPtEtaManualPtMinMax = (debugFilterkAcceptaceManualPtMinMax.node() == false) || ((aod::track::pt > 0.1f) && (aod::track::pt < 1e10f) && (nabs(aod::track::eta) < 0.8f));
  Partition<aod::TracksIU> tracksIUFiltered = (debugFilterkAcceptaceManualPtMin.node() == false && debugFilterkAcceptaceManualPtMinMax.node() == false) || (debugFilterkAcceptaceManualPtMin.node() == true && (aod::track::pt > 0.1f) && (nabs(aod::track::eta) < 0.8f)) || (debugFilterkAcceptaceManualPtMinMax.node() == true && (aod::track::pt > 0.1f) && (aod::track::pt < 1e10f) && (nabs(aod::track::eta) < 0.8f));

  HistogramRegistry histos;

  void init(InitContext const&);

  // Function to select tracks
  template <bool IS_MC, typename T>
  bool isSelectedTrack(const T& track);

  // Function to select collisions
  template <bool doFill, typename T>
  bool isSelectedCollision(const T& collision)
  {
    if constexpr (doFill) {
      histos.fill(HIST("Events/recoEff"), 1);
    }
    if (selectGoodEvents && !(isRun3 ? collision.sel8() : collision.sel7())) { // currently only sel8 is defined for run3
      return false;
    }
    if constexpr (doFill) {
      histos.fill(HIST("Events/recoEff"), 2);
    }
    return true;
  }

  // General function to fill data and MC histograms
  template <bool IS_MC, typename C, typename T>
  void fillRecoHistograms(const C& collision, const T& tracks, aod::FullTracks const& tracksUnfiltered);

  // Process function for data
  using CollisionTableData = soa::Join<aod::Collisions, aod::EvSels>;
  using TrackTableData = soa::Filtered<soa::Join<aod::FullTracks, aod::TracksCov, aod::TracksDCA, aod::TrackSelection, aod::TOFSignal, aod::TOFEvTime>>;
  void processData(CollisionTableData::iterator const& collision, TrackTableData const& tracks, aod::FullTracks const& tracksUnfiltered)
  {
    fillRecoHistograms<false>(collision, tracks, tracksUnfiltered);
  }
  PROCESS_SWITCH(qaEventTrack, processData, "process data", false);

  // Process function for IU vs DCA track comparison
  void processDataIU(CollisionTableData::iterator const& collision,
                     aod::FullTracks const& tracksUnfiltered, aod::TracksIU const& tracksIU)
  {
    if (!isSelectedCollision<false>(collision)) {
      return;
    }

    int trackIndex = 0;
    for (const auto& trk : tracksUnfiltered) {
      if (!isSelectedTrack<false>(trk)) {
        continue;
      }

      const auto& trkIU = tracksIU.iteratorAt(trackIndex++);
      histos.fill(HIST("Tracks/IU/Pt"), trkIU.pt());
      histos.fill(HIST("Tracks/IU/Eta"), trkIU.eta());
      histos.fill(HIST("Tracks/IU/Phi"), trkIU.phi());

      histos.fill(HIST("Tracks/IU/alpha"), trkIU.alpha());
      histos.fill(HIST("Tracks/IU/x"), trkIU.x());
      histos.fill(HIST("Tracks/IU/y"), trkIU.y());
      histos.fill(HIST("Tracks/IU/z"), trkIU.z());
      histos.fill(HIST("Tracks/IU/signed1Pt"), trkIU.signed1Pt());
      histos.fill(HIST("Tracks/IU/snp"), trkIU.snp());
      histos.fill(HIST("Tracks/IU/tgl"), trkIU.tgl());

      histos.fill(HIST("Tracks/IUdeltaDCA/Pt"), trkIU.pt() - trk.pt());
      histos.fill(HIST("Tracks/IUdeltaDCA/Eta"), trkIU.eta() - trk.eta());
      histos.fill(HIST("Tracks/IUdeltaDCA/Phi"), trkIU.phi() - trk.phi());

      histos.fill(HIST("Tracks/IUvsDCA/Pt"), trk.pt(), trkIU.pt());
      histos.fill(HIST("Tracks/IUvsDCA/Eta"), trk.eta(), trkIU.eta());
      histos.fill(HIST("Tracks/IUvsDCA/Phi"), trk.phi(), trkIU.phi());
    }
  }
  PROCESS_SWITCH(qaEventTrack, processDataIU, "process IU vs DCA comparison", true);

  // Process function for filtered IU
  void processDataIUFiltered(CollisionTableData::iterator const& collision)
  {
    if (!isSelectedCollision<false>(collision)) {
      return;
    }

    auto tracksIU = tracksIUFiltered->sliceByCached(aod::track::collisionId, collision.globalIndex());

    // int trackIndex = 0;
    for (const auto& trkIU : tracksIU) {
      if (!isSelectedTrack<false>(trkIU)) {
        continue;
      }

      // const auto& trkIU = tracksIU.iteratorAt(trackIndex++);
      histos.fill(HIST("Tracks/IUFiltered/Pt"), trkIU.pt());
      histos.fill(HIST("Tracks/IUFiltered/Eta"), trkIU.eta());
      histos.fill(HIST("Tracks/IUFiltered/Phi"), trkIU.phi());

      histos.fill(HIST("Tracks/IUFiltered/alpha"), trkIU.alpha());
      histos.fill(HIST("Tracks/IUFiltered/x"), trkIU.x());
      histos.fill(HIST("Tracks/IUFiltered/y"), trkIU.y());
      histos.fill(HIST("Tracks/IUFiltered/z"), trkIU.z());
      histos.fill(HIST("Tracks/IUFiltered/signed1Pt"), trkIU.signed1Pt());
      histos.fill(HIST("Tracks/IUFiltered/snp"), trkIU.snp());
      histos.fill(HIST("Tracks/IUFiltered/tgl"), trkIU.tgl());
    }
  }
  PROCESS_SWITCH(qaEventTrack, processDataIUFiltered, "process IU filtered", true);

  // Process function for MC
  using CollisionTableMC = soa::Join<aod::Collisions, aod::McCollisionLabels, aod::EvSels>;
  using TrackTableMC = soa::Filtered<soa::Join<aod::FullTracks, aod::TracksCov, aod::TracksDCA, aod::TrackSelection, aod::TOFSignal, aod::TOFEvTime,
                                               aod::McTrackLabels>>;
  void processMC(CollisionTableMC::iterator const& collision, TrackTableMC const& tracks, aod::FullTracks const& tracksUnfiltered,
                 aod::McParticles const& mcParticles, aod::McCollisions const& mcCollisions)
  {
    fillRecoHistograms<true>(collision, tracks, tracksUnfiltered);
  }
  PROCESS_SWITCH(qaEventTrack, processMC, "process mc", true); // FIXME: would like to disable this by default and swich on via --processMC but currently this crashes -> ask experts

  // Process functions for skimming data
  void processTableData(CollisionTableData::iterator const& collision,
                        TrackTableData const& tracks,
                        aod::BCs const& bcs)
  {
    fillDerivedTable<false>(collision, tracks, 0, bcs);
  }
  PROCESS_SWITCH(qaEventTrack, processTableData, "Process data for table producing", false);

  void processTableMC(CollisionTableMC::iterator const& collision,
                      TrackTableMC const& tracks,
                      aod::McParticles const& mcParticles,
                      aod::McCollisions const& mcCollisions,
                      aod::BCs const& bcs)
  {
    fillDerivedTable<true>(collision, tracks, mcParticles, bcs);
  }
  PROCESS_SWITCH(qaEventTrack, processTableMC, "Process MC for table producing", false);

  //**************************************************************************************************
  /**
   * Fill reco level tables.
   */
  //**************************************************************************************************
  int nTableEventCounter = 0; // Number of processed events
  template <bool IS_MC, typename C, typename T, typename P>
  void fillDerivedTable(const C& collision, const T& tracks, const P& particles, const aod::BCs&)
  {
    if (!isSelectedCollision<false>(collision)) {
      return;
    }
    if (abs(collision.posZ()) > selectMaxVtxZ) {
      return;
    }
    if (fractionOfSampledEvents < 1.f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) > fractionOfSampledEvents) { // Skip events that are not sampled
      return;
    }
    if (nTableEventCounter > targetNumberOfEvents) { // Skip events if target is reached
      return;
    }
    nTableEventCounter++;

    tableCollisions(collision.posZ(),
                    (isRun3 ? collision.sel8() : collision.sel7()),
                    collision.bc().runNumber());
    int nTracks = 0;
    int particleProduction = 0;

    for (const auto& track : tracks) {
      if (!isSelectedTrack<IS_MC>(track)) {
        continue;
      }
      ++nTracks;
    }
    tableTracks.reserve(nTracks);
    std::vector<int64_t> recoPartIndices(IS_MC ? nTracks : 0);

    if constexpr (IS_MC) { // Running only on MC
      tableRecoParticles.reserve(nTracks);
    }
    int64_t iTrack = 0;
    for (const auto& track : tracks) {
      if (!isSelectedTrack<IS_MC>(track)) {
        continue;
      }
      tableTracks(tableCollisions.lastIndex(),
                  track.pt(), track.eta(), track.phi(), track.pt() * std::sqrt(track.c1Pt21Pt2()),
                  track.flags(), track.sign(),
                  track.dcaXY(), track.dcaZ(), track.length(),
                  track.itsClusterMap(),
                  track.itsChi2NCl(), track.tpcChi2NCl(), track.trdChi2(), track.tofChi2(),
                  track.hasITS(), track.hasTPC(), track.hasTRD(), track.hasTOF(),
                  track.tpcNClsFound(), track.tpcNClsCrossedRows(),
                  track.tpcCrossedRowsOverFindableCls(), track.tpcFoundOverFindableCls(), track.tpcFractionSharedCls(),
                  track.itsNCls(), track.itsNClsInnerBarrel(), track.tpcSignal(), track.tofSignal() - track.tofEvTime());

      if constexpr (IS_MC) { // Running only on MC
        if (track.has_mcParticle()) {
          auto particle = track.mcParticle();
          recoPartIndices[iTrack++] = particle.globalIndex();
          if (particle.isPhysicalPrimary()) {
            particleProduction = 0;
          } else if (particle.getProcess() == 4) {
            particleProduction = 1;
          } else {
            particleProduction = 2;
          }
          tableRecoParticles(particle.pt(), particle.eta(), particle.phi(),
                             particle.pdgCode(), particleProduction);
        } else { // If it does not have the particle we fill with the track values and tag it with -1 in the production
          tableRecoParticles(track.pt(), track.eta(), track.phi(),
                             0, -1);
        }
      }
    }

    // Running only on MC
    if constexpr (IS_MC) {
      if (!collision.has_mcCollision()) {
        return;
      }
      const auto& particlesInCollision = particles.sliceBy(aod::mcparticle::mcCollisionId, collision.mcCollision().globalIndex());
      tableNonRecoParticles.reserve(particlesInCollision.size() - nTracks);
      for (const auto& particle : particlesInCollision) {
        const auto partReconstructed = std::find(recoPartIndices.begin(), recoPartIndices.end(), particle.globalIndex()) != recoPartIndices.end();
        if (partReconstructed) {
          continue;
        }
        if (particle.isPhysicalPrimary()) {
          particleProduction = 0;
        } else if (particle.getProcess() == 4) {
          particleProduction = 1;
        } else {
          particleProduction = 2;
        }
        tableNonRecoParticles(tableCollisions.lastIndex(),
                              particle.pt(), particle.eta(), particle.phi(),
                              particle.pdgCode(), particleProduction,
                              particle.vx(), particle.vy(), particle.vz());
      }
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<qaEventTrack>(cfgc)};
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// Task implementation
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Initialize the task.
 */
//**************************************************************************************************
void qaEventTrack::init(InitContext const&)
{
  if (doprocessTableData == true && doprocessTableMC == true) {
    LOGF(fatal, "Cannot enable processTableData and processTableMC at the same time. Please choose one.");
  }
  if (!doprocessData && !doprocessMC) {
    LOGF(info, "No enabled QA, all histograms are disabled");
    return;
  }
  const AxisSpec axisPt{binsPt, "#it{p}_{T} [GeV/c]"};
  const AxisSpec axisEta{180, -0.9, 0.9, "#it{#eta}"};
  const AxisSpec axisPhi{180, 0., 2 * M_PI, "#it{#varphi} [rad]"};
  const AxisSpec axisVertexNumContrib{200, 0, 200, "Number Of contributors to the PV"};
  const AxisSpec axisVertexPosX{binsVertexPosXY, "X [cm]"};
  const AxisSpec axisVertexPosY{binsVertexPosXY, "Y [cm]"};
  const AxisSpec axisVertexPosZ{binsVertexPosZ, "Z [cm]"};
  const AxisSpec axisVertexCov{100, -0.005, 0.005};
  const AxisSpec axisVertexPosReso{100, -0.5, 0.5};
  const AxisSpec axisTrackMultiplicity{binsTrackMultiplicity, "Track Multiplicity"};
  const AxisSpec axisParX{200, -0.36, 0.36, "#it{x} [cm]"};
  const AxisSpec axisParY{200, -0.5, 0.5, "#it{y} [cm]"};
  const AxisSpec axisParZ{200, -11., 11., "#it{z} [cm]"};
  const AxisSpec axisParAlpha{36, -M_PI, M_PI, "#alpha [rad]"};
  const AxisSpec axisParSigned1Pt{200, -8, 8, "#it{q}/#it{p}_{T}"};
  const AxisSpec axisParSnp{11, -0.1, 0.1, "snp"};
  const AxisSpec axisParTgl{200, -1., 1., "tgl"};

  const AxisSpec axisDeltaPt{100, -0.5, 0.5, "#it{p}_{T, rec} - #it{p}_{T, gen}"};
  const AxisSpec axisDeltaEta{100, -0.1, 0.1, "#eta_{rec} - #eta_{gen}"};
  const AxisSpec axisDeltaPhi{100, -0.1, 0.1, "#phi_{rec} - #phi_{gen}"};

  // collision
  auto eventRecoEffHist = histos.add<TH1>("Events/recoEff", "", kTH1D, {{2, 0.5, 2.5}});
  eventRecoEffHist->GetXaxis()->SetBinLabel(1, "all");
  eventRecoEffHist->GetXaxis()->SetBinLabel(2, "selected");
  histos.add("Events/posX", "", kTH1D, {axisVertexPosX});
  histos.add("Events/posY", "", kTH1D, {axisVertexPosY});
  histos.add("Events/posZ", "", kTH1D, {axisVertexPosZ});
  histos.add("Events/posXY", "", kTH2D, {axisVertexPosX, axisVertexPosY});
  histos.add("Events/posXvsNContrib", "", kTH2D, {axisVertexPosX, axisVertexNumContrib});
  histos.add("Events/posYvsNContrib", "", kTH2D, {axisVertexPosY, axisVertexNumContrib});
  histos.add("Events/posZvsNContrib", "", kTH2D, {axisVertexPosZ, axisVertexNumContrib});
  histos.add("Events/nContrib", "", kTH1D, {axisVertexNumContrib});
  histos.add("Events/nContribVsMult", "", kTH2D, {axisVertexNumContrib, axisTrackMultiplicity});
  histos.add("Events/vertexChi2", ";#chi^{2}", kTH1D, {{100, 0, 100}});

  histos.add("Events/covXX", ";Cov_{xx} [cm^{2}]", kTH1D, {axisVertexCov});
  histos.add("Events/covXY", ";Cov_{xy} [cm^{2}]", kTH1D, {axisVertexCov});
  histos.add("Events/covXZ", ";Cov_{xz} [cm^{2}]", kTH1D, {axisVertexCov});
  histos.add("Events/covYY", ";Cov_{yy} [cm^{2}]", kTH1D, {axisVertexCov});
  histos.add("Events/covYZ", ";Cov_{yz} [cm^{2}]", kTH1D, {axisVertexCov});
  histos.add("Events/covZZ", ";Cov_{zz} [cm^{2}]", kTH1D, {axisVertexCov});

  histos.add("Events/nTracks", "", kTH1D, {axisTrackMultiplicity});

  if (doprocessMC) {
    histos.add("Events/resoX", ";X_{Rec} - X_{Gen} [cm]", kTH2D, {axisVertexPosReso, axisVertexNumContrib});
    histos.add("Events/resoY", ";Y_{Rec} - Y_{Gen} [cm]", kTH2D, {axisVertexPosReso, axisVertexNumContrib});
    histos.add("Events/resoZ", ";Z_{Rec} - Z_{Gen} [cm]", kTH2D, {axisVertexPosReso, axisVertexNumContrib});
  }

  auto trackRecoEffHist = histos.add<TH1>("Tracks/recoEff", "", kTH1D, {{2, 0.5, 2.5}});
  trackRecoEffHist->GetXaxis()->SetBinLabel(1, "all");
  trackRecoEffHist->GetXaxis()->SetBinLabel(2, "selected");
  trackRecoEffHist->SetBit(TH1::kIsNotW);

  // kine histograms
  histos.add("Tracks/Kine/pt", "#it{p}_{T};#it{p}_{T} [GeV/c]", kTH1D, {{axisPt}});
  histos.add("Tracks/Kine/eta", "#eta", kTH1D, {axisEta});
  histos.add("Tracks/Kine/phi", "#phi", kTH1D, {axisPhi});
  if (doprocessMC) {
    histos.add("Tracks/Kine/resoPt", "", kTH2D, {axisDeltaPt, axisPt});
    histos.add<TH2>("Tracks/Kine/resoEta", "", kTH2D, {axisDeltaEta, axisEta})->GetYaxis()->SetTitle("#eta_{rec}");
    histos.add<TH2>("Tracks/Kine/resoPhi", "", kTH2D, {axisDeltaPhi, axisPhi})->GetYaxis()->SetTitle("#phi_{rec}");
  }
  histos.add("Tracks/Kine/relativeResoPt", "relative #it{p}_{T} resolution;#sigma{#it{p}}/#it{p}_{T};#it{p}_{T}", kTH2D, {{axisPt, {100, 0., 0.3}}});
  histos.add("Tracks/Kine/relativeResoPtMean", "mean relative #it{p}_{T} resolution;#LT#sigma{#it{p}}/#it{p}_{T}#GT;#it{p}_{T}", kTProfile, {{axisPt}});

  // track histograms
  histos.add("Tracks/x", "track #it{x} position at dca in local coordinate system", kTH1D, {axisParX});
  histos.add("Tracks/y", "track #it{y} position at dca in local coordinate system", kTH1D, {axisParY});
  histos.add("Tracks/z", "track #it{z} position at dca in local coordinate system", kTH1D, {axisParZ});
  histos.add("Tracks/alpha", "rotation angle of local wrt. global coordinate system", kTH1D, {axisParAlpha});
  histos.add("Tracks/signed1Pt", "track signed 1/#it{p}_{T}", kTH1D, {axisParSigned1Pt});
  histos.add("Tracks/snp", "sinus of track momentum azimuthal angle", kTH1D, {axisParSnp});
  histos.add("Tracks/tgl", "tangent of the track momentum dip angle", kTH1D, {axisParTgl});
  histos.add("Tracks/flags", "track flag;flag bit", kTH1D, {{64, -0.5, 63.5}});
  histos.add("Tracks/dcaXY", "distance of closest approach in #it{xy} plane;#it{dcaXY} [cm];", kTH1D, {{200, -0.15, 0.15}});
  histos.add("Tracks/dcaZ", "distance of closest approach in #it{z};#it{dcaZ} [cm];", kTH1D, {{200, -0.15, 0.15}});

  histos.add("Tracks/dcaXYvsPt", "distance of closest approach in #it{xy} plane;#it{dcaXY} [cm];", kTH2D, {{200, -0.15, 0.15}, axisPt});
  histos.add("Tracks/dcaZvsPt", "distance of closest approach in #it{z};#it{dcaZ} [cm];", kTH2D, {{200, -0.15, 0.15}, axisPt});

  histos.add("Tracks/length", "track length in cm;#it{Length} [cm];", kTH1D, {{400, 0, 1000}});

  // its histograms
  histos.add("Tracks/ITS/itsNCls", "number of found ITS clusters;# clusters ITS", kTH1D, {{8, -0.5, 7.5}});
  histos.add("Tracks/ITS/itsChi2NCl", "chi2 per ITS cluster;chi2 / cluster ITS", kTH1D, {{100, 0, 40}});
  histos.add("Tracks/ITS/itsHits", "No. of hits vs ITS layer;layer ITS", kTH2D, {{8, -1.5, 6.5}, {8, -0.5, 7.5, "No. of hits"}});
  histos.add("Tracks/ITS/itsHitsUnfiltered", "No. of hits vs ITS layer (unfiltered tracks);layer ITS", kTH2D, {{8, -1.5, 6.5}, {8, -0.5, 7.5, "No. of hits"}});
  histos.add("Tracks/ITS/hasITS", "pt distribution of tracks crossing ITS", kTH1D, {axisPt});
  histos.add("Tracks/ITS/hasITSANDhasTPC", "pt distribution of tracks crossing both ITS and TPC", kTH1D, {axisPt});

  // tpc histograms
  histos.add("Tracks/TPC/tpcNClsFindable", "number of findable TPC clusters;# findable clusters TPC", kTH1D, {{165, -0.5, 164.5}});
  histos.add("Tracks/TPC/tpcNClsFound", "number of found TPC clusters;# clusters TPC", kTH1D, {{165, -0.5, 164.5}});
  histos.add("Tracks/TPC/tpcNClsShared", "number of shared TPC clusters;# shared clusters TPC", kTH1D, {{165, -0.5, 164.5}});
  histos.add("Tracks/TPC/tpcCrossedRows", "number of crossed TPC rows;# crossed rows TPC", kTH1D, {{165, -0.5, 164.5}});
  histos.add("Tracks/TPC/tpcFractionSharedCls", "fraction of shared TPC clusters;fraction shared clusters TPC", kTH1D, {{100, 0., 1.}});
  histos.add("Tracks/TPC/tpcCrossedRowsOverFindableCls", "crossed TPC rows over findable clusters;crossed rows / findable clusters TPC", kTH1D, {{60, 0.7, 1.3}});
  histos.add("Tracks/TPC/tpcChi2NCl", "chi2 per cluster in TPC;chi2 / cluster TPC", kTH1D, {{100, 0, 10}});
  histos.add("Tracks/TPC/hasTPC", "pt distribution of tracks crossing TPC", kTH1D, {axisPt});

  // tracks vs tracks @ IU
  if (doprocessDataIU) {
    // Full distributions
    auto h1 = histos.add<TH1>("Tracks/IU/Pt", "IU: Pt", kTH1F, {axisPt});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/Eta", "IU: Eta", kTH1F, {axisEta});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/Phi", "IU: Phi", kTH1F, {axisPhi});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));

    h1 = histos.add<TH1>("Tracks/IU/x", "IU: x", kTH1F, {axisParX});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/y", "IU: y", kTH1F, {axisParY});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/z", "IU: z", kTH1F, {axisParZ});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/alpha", "rotation angle of local wrt. global coordinate system", kTH1F, {axisParAlpha});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/signed1Pt", "track signed 1/#it{p}_{T}", kTH1F, {axisParSigned1Pt});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/snp", "sinus of track momentum azimuthal angle", kTH1F, {axisParSnp});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IU/tgl", "tangent of the track momentum dip angle", kTH1F, {axisParTgl});
    h1->GetXaxis()->SetTitle(Form("%s IU", h1->GetXaxis()->GetTitle()));

    // Deltas
    h1 = histos.add<TH1>("Tracks/IUdeltaDCA/Pt", "IU - DCA: Pt", kTH1F, {axisPt});
    h1->GetXaxis()->SetTitle(Form("%s IU - DCA", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUdeltaDCA/Eta", "IU - DCA: Eta", kTH1F, {axisEta});
    h1->GetXaxis()->SetTitle(Form("%s IU - DCA", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUdeltaDCA/Phi", "IU - DCA: Phi", kTH1F, {axisPhi});
    h1->GetXaxis()->SetTitle(Form("%s IU - DCA", h1->GetXaxis()->GetTitle()));
    // Correlations
    auto h2 = histos.add<TH2>("Tracks/IUvsDCA/Pt", "IU vs DCA: Pt", kTH2F, {axisPt, axisPt});
    h2->GetXaxis()->SetTitle(Form("%s DCA", h2->GetXaxis()->GetTitle()));
    h2->GetYaxis()->SetTitle(Form("%s IU", h2->GetYaxis()->GetTitle()));
    h2 = histos.add<TH2>("Tracks/IUvsDCA/Eta", "IU vs DCA: Eta", kTH2F, {axisEta, axisEta});
    h2->GetXaxis()->SetTitle(Form("%s DCA", h2->GetXaxis()->GetTitle()));
    h2->GetYaxis()->SetTitle(Form("%s IU", h2->GetYaxis()->GetTitle()));
    h2 = histos.add<TH2>("Tracks/IUvsDCA/Phi", "IU vs DCA: Phi", kTH2F, {axisPhi, axisPhi});
    h2->GetXaxis()->SetTitle(Form("%s DCA", h2->GetXaxis()->GetTitle()));
    h2->GetYaxis()->SetTitle(Form("%s IU", h2->GetYaxis()->GetTitle()));
  }

  // filtered tracks @ IU
  if (doprocessDataIUFiltered) {
    // Full distributions
    auto h1 = histos.add<TH1>("Tracks/IUFiltered/Pt", "IU: Pt", kTH1F, {axisPt});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/Eta", "IU: Eta", kTH1F, {axisEta});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/Phi", "IU: Phi", kTH1F, {axisPhi});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));

    h1 = histos.add<TH1>("Tracks/IUFiltered/x", "IU: x", kTH1F, {axisParX});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/y", "IU: y", kTH1F, {axisParY});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/z", "IU: z", kTH1F, {axisParZ});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/alpha", "rotation angle of local wrt. global coordinate system", kTH1F, {axisParAlpha});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/signed1Pt", "track signed 1/#it{p}_{T}", kTH1F, {axisParSigned1Pt});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/snp", "sinus of track momentum azimuthal angle", kTH1F, {axisParSnp});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
    h1 = histos.add<TH1>("Tracks/IUFiltered/tgl", "tangent of the track momentum dip angle", kTH1F, {axisParTgl});
    h1->GetXaxis()->SetTitle(Form("%s IU filtered", h1->GetXaxis()->GetTitle()));
  }

  // tracks IU filtered
}

//**************************************************************************************************
/**
 * Check if track fulfils the configurable requirements.
 */
//**************************************************************************************************
template <bool IS_MC, typename T>
bool qaEventTrack::isSelectedTrack(const T& track)
{
  if (selectCharge && (selectCharge != track.sign())) {
    return false;
  }
  if constexpr (IS_MC) {
    if (!track.has_mcParticle()) {
      if (selectPrim || selectSec || selectPID) {
        return false;
      } else {
        return true;
      }
    }
    auto particle = track.mcParticle();
    const bool isPrimary = particle.isPhysicalPrimary();
    if (selectPrim && !isPrimary) {
      return false;
    }
    if (selectSec && isPrimary) {
      return false;
    }
    if (selectPID && selectPID != std::abs(particle.pdgCode())) {
      return false;
    }
  }
  return true;
}

//**************************************************************************************************
/**
 * Fill reco level histograms.
 */
//**************************************************************************************************
template <bool IS_MC, typename C, typename T>
void qaEventTrack::fillRecoHistograms(const C& collision, const T& tracks, const aod::FullTracks& tracksUnfiltered)
{
  // fill reco collision related histograms
  if (!isSelectedCollision<true>(collision)) {
    return;
  }

  int nTracks = 0;
  for (const auto& track : tracks) {
    if (!isSelectedTrack<IS_MC>(track)) {
      continue;
    }
    ++nTracks;
  }

  histos.fill(HIST("Events/posX"), collision.posX());
  histos.fill(HIST("Events/posY"), collision.posY());
  histos.fill(HIST("Events/posZ"), collision.posZ());
  histos.fill(HIST("Events/posXY"), collision.posX(), collision.posY());

  histos.fill(HIST("Events/posXvsNContrib"), collision.posX(), collision.numContrib());
  histos.fill(HIST("Events/posYvsNContrib"), collision.posY(), collision.numContrib());
  histos.fill(HIST("Events/posZvsNContrib"), collision.posZ(), collision.numContrib());

  histos.fill(HIST("Events/nContrib"), collision.numContrib());
  histos.fill(HIST("Events/nContribVsMult"), collision.numContrib(), nTracks);
  histos.fill(HIST("Events/vertexChi2"), collision.chi2());

  histos.fill(HIST("Events/covXX"), collision.covXX());
  histos.fill(HIST("Events/covXY"), collision.covXY());
  histos.fill(HIST("Events/covXZ"), collision.covXZ());
  histos.fill(HIST("Events/covYY"), collision.covYY());
  histos.fill(HIST("Events/covYZ"), collision.covYZ());
  histos.fill(HIST("Events/covZZ"), collision.covZZ());

  histos.fill(HIST("Events/nTracks"), nTracks);

  // vertex resolution
  if constexpr (IS_MC) {
    if (collision.has_mcCollision()) {
      const auto mcColl = collision.mcCollision();
      histos.fill(HIST("Events/resoX"), collision.posX() - mcColl.posX(), collision.numContrib());
      histos.fill(HIST("Events/resoY"), collision.posY() - mcColl.posY(), collision.numContrib());
      histos.fill(HIST("Events/resoZ"), collision.posZ() - mcColl.posZ(), collision.numContrib());
    }
  }

  histos.fill(HIST("Tracks/recoEff"), 1, tracks.tableSize());
  histos.fill(HIST("Tracks/recoEff"), 2, tracks.size());

  // unfiltered track related histograms
  for (const auto& trackUnfiltered : tracksUnfiltered) {
    // fill ITS variables
    int itsNhits = 0;
    for (unsigned int i = 0; i < 7; i++) {
      if (trackUnfiltered.itsClusterMap() & (1 << i)) {
        itsNhits += 1;
      }
    }
    bool trkHasITS = false;
    for (unsigned int i = 0; i < 7; i++) {
      if (trackUnfiltered.itsClusterMap() & (1 << i)) {
        trkHasITS = true;
        histos.fill(HIST("Tracks/ITS/itsHitsUnfiltered"), i, itsNhits);
      }
    }
    if (!trkHasITS) {
      histos.fill(HIST("Tracks/ITS/itsHitsUnfiltered"), -1, itsNhits);
    }
  }

  // track related histograms
  for (const auto& track : tracks) {
    if (!isSelectedTrack<IS_MC>(track)) {
      continue;
    }
    // fill kinematic variables
    histos.fill(HIST("Tracks/Kine/pt"), track.pt());
    histos.fill(HIST("Tracks/Kine/eta"), track.eta());
    histos.fill(HIST("Tracks/Kine/phi"), track.phi());
    histos.fill(HIST("Tracks/Kine/relativeResoPt"), track.pt(), track.pt() * std::sqrt(track.c1Pt21Pt2()));
    histos.fill(HIST("Tracks/Kine/relativeResoPtMean"), track.pt(), track.pt() * std::sqrt(track.c1Pt21Pt2()));

    // fill track parameters
    histos.fill(HIST("Tracks/alpha"), track.alpha());
    histos.fill(HIST("Tracks/x"), track.x());
    histos.fill(HIST("Tracks/y"), track.y());
    histos.fill(HIST("Tracks/z"), track.z());
    histos.fill(HIST("Tracks/signed1Pt"), track.signed1Pt());
    histos.fill(HIST("Tracks/snp"), track.snp());
    histos.fill(HIST("Tracks/tgl"), track.tgl());
    for (unsigned int i = 0; i < 64; i++) {
      if (track.flags() & (1 << i)) {
        histos.fill(HIST("Tracks/flags"), i);
      }
    }
    histos.fill(HIST("Tracks/dcaXY"), track.dcaXY());
    histos.fill(HIST("Tracks/dcaZ"), track.dcaZ());
    histos.fill(HIST("Tracks/dcaXYvsPt"), track.dcaXY(), track.pt());
    histos.fill(HIST("Tracks/dcaZvsPt"), track.dcaZ(), track.pt());
    histos.fill(HIST("Tracks/length"), track.length());

    // fill ITS variables
    histos.fill(HIST("Tracks/ITS/itsNCls"), track.itsNCls());
    histos.fill(HIST("Tracks/ITS/itsChi2NCl"), track.itsChi2NCl());
    int itsNhits = 0;
    for (unsigned int i = 0; i < 7; i++) {
      if (track.itsClusterMap() & (1 << i)) {
        itsNhits += 1;
      }
    }
    bool trkHasITS = false;
    for (unsigned int i = 0; i < 7; i++) {
      if (track.itsClusterMap() & (1 << i)) {
        trkHasITS = true;
        histos.fill(HIST("Tracks/ITS/itsHits"), i, itsNhits);
      }
    }
    if (!trkHasITS) {
      histos.fill(HIST("Tracks/ITS/itsHits"), -1, itsNhits);
    }

    // fill TPC variables
    histos.fill(HIST("Tracks/TPC/tpcNClsFindable"), track.tpcNClsFindable());
    histos.fill(HIST("Tracks/TPC/tpcNClsFound"), track.tpcNClsFound());
    histos.fill(HIST("Tracks/TPC/tpcNClsShared"), track.tpcNClsShared());
    histos.fill(HIST("Tracks/TPC/tpcCrossedRows"), track.tpcNClsCrossedRows());
    histos.fill(HIST("Tracks/TPC/tpcCrossedRowsOverFindableCls"), track.tpcCrossedRowsOverFindableCls());
    histos.fill(HIST("Tracks/TPC/tpcFractionSharedCls"), track.tpcFractionSharedCls());
    histos.fill(HIST("Tracks/TPC/tpcChi2NCl"), track.tpcChi2NCl());

    if constexpr (IS_MC) {
      if (track.has_mcParticle()) {
        // resolution plots
        auto particle = track.mcParticle();
        histos.fill(HIST("Tracks/Kine/resoPt"), track.pt() - particle.pt(), track.pt());
        histos.fill(HIST("Tracks/Kine/resoEta"), track.eta() - particle.eta(), track.eta());
        histos.fill(HIST("Tracks/Kine/resoPhi"), track.phi() - particle.phi(), track.phi());
      }
    }

    // ITS-TPC matching pt-distributions
    if (track.hasITS()) {
      histos.fill(HIST("Tracks/ITS/hasITS"), track.pt());
    }
    if (track.hasTPC()) {
      histos.fill(HIST("Tracks/TPC/hasTPC"), track.pt());
    }
    if (track.hasITS() && track.hasTPC()) {
      histos.fill(HIST("Tracks/ITS/hasITSANDhasTPC"), track.pt());
    }
  }
}
