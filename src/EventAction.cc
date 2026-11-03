#include "EventAction.hh"

#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"


//=====================================================
//========= To extract P1 and P2 to CSV ===============
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4MaterialPropertyVector.hh"
#include <cmath>
//=====================================================

//===== For mac-controls ==============================
#include "G4GenericMessenger.hh"
#include "G4StateManager.hh"
#include <cstdlib>
//=====================================================


#include <iomanip>
#include <filesystem>
#include <string>

#include "G4PrimaryVertex.hh"              //Added for REcording gamma launch coordinates

namespace {
std::string outputPath(const char* filename)
{
    const std::filesystem::path directory(TRIGGER_OUTPUT_DIR);
    std::filesystem::create_directories(directory);
    return (directory / filename).string();
}
}
// 4 lines below replaced for mac-controls
// EventAction::EventAction()
//     : fOutput(outputPath("photons.csv")),
//       fSummary(outputPath("photon_summary.csv"))
// {
EventAction::EventAction()
{
    OpenOutput();

    fMessenger =
        new G4GenericMessenger(this, "/scan/", "Scan output lifecycle");

    fMessenger->DeclareMethod("begin", &EventAction::BeginScan)
        .SetStates(G4State_Idle);

    fMessenger->DeclareMethod("saveAndReset", &EventAction::SaveAndReset)
        .SetStates(G4State_Idle);

    fMessenger->DeclareMethod("finish", &EventAction::FinishScan)
        .SetStates(G4State_Idle);
}

void EventAction::OpenOutput()
{
    fOutput.open(outputPath("Collected_Data.csv"), std::ios::trunc);
    fSummary.open(outputPath("Real_Data.csv"), std::ios::trunc);
    if (!fOutput.is_open() || !fSummary.is_open())
    {
        G4Exception(
            "EventAction::EventAction",
            "OutputOpenFailed",
            FatalException,
            "Cannot open photon OUTPUT FILES.csv"
        );
    }

    //fSummary << "run_id,event_id,generated,absorbed,collected\n"; //FOR DATA COLLECTION, replaced by bottom 4 lines
    fSummary
    << "run_id,event_id,source_x_mm,source_y_mm,source_z_mm,"
    << "generated,absorbed,collected,"
    << "p1_photons_per_MeV,p2_cm\n";
    fSummary << std::setprecision(10);


    fOutput << std::setprecision(10);

    fOutput
        << "run_id,event_id,sensor_id,photon_count,energies_eV\n";
}
//======|||===|||=============================================================================================================|||===|||===|||
EventAction::~EventAction()
{
    delete fMessenger;
}

void EventAction::RunAnalysis(const char* mode)
{
    const std::string command =
        std::string("python3 \"") + TRIGGER_PROJECT_DIR
        + "/python/Middleman.py\" " + mode;

    if (std::system(command.c_str()) != 0)
    {
        G4Exception(
            "RunAnalysis",
            "ScanPipelineFailed",
            FatalException,
            "Collection/plotting failed; scan stopped. Temporary CSVs retained."
        );
    }
}

void EventAction::BeginScan()
{
    if (fScanActive)
    {
        G4Exception(
            "BeginScan",
            "ScanAlreadyActive",
            FatalException,
            "Scan already active"
        );
    }

    RunAnalysis("begin");

    fOutput.close();
    fSummary.close();
    OpenOutput();

    fScanActive = true;
}

void EventAction::SaveAndReset()
{
    if (!fScanActive)
    {
        G4Exception(
            "SaveAndReset",
            "NoActiveScan",
            FatalException,
            "Call /scan/begin first"
        );
    }

    fOutput.flush();
    fSummary.flush();

    if (!fOutput || !fSummary)
    {
        G4Exception(
            "SaveAndReset",
            "OutputFailed",
            FatalException,
            "Cannot flush output"
        );
    }

    RunAnalysis("save");

    fOutput.close();
    fSummary.close();
    OpenOutput();
}

void EventAction::FinishScan()
{
    if (!fScanActive)
    {
        G4Exception(
            "FinishScan",
            "NoActiveScan",
            FatalException,
            "Call /scan/begin first"
        );
    }

    RunAnalysis("finish");
    fScanActive = false;
}
//======|||===|||=============================================================================================================|||===|||===|||
void EventAction::BeginOfEventAction(const G4Event*)
{
    //FOR DATA COLLECTION:
    fGenerated = 0;
    fAbsorbed = 0;

    // Empty the lists, retaining their allocated memory.
    for (auto& energies : fPhotonEnergies)
        energies.clear();
}

void EventAction::RecordPhoton(G4int sensorID, G4double energy)
{
    if (sensorID < 0 || sensorID >= 100) //0-100 error, change if the no. is chnaged
    {
        G4Exception(
            "EventAction::RecordPhoton",
            "InvalidSensorID",
            FatalException,
            "SiPM ID must be between 0 and 99"
        );
        return;
    }

    fPhotonEnergies[sensorID].push_back(energy);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    const auto runID =
        G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();

    // Get the vertex where this event's gamma was launched. (BOTTOM 12 LINES until } added for recording gamma launch coordinates)
    const auto* sourceVertex = event->GetPrimaryVertex();

    if (!sourceVertex)
    {
        G4Exception(
            "EventAction::EndOfEventAction",
            "MissingPrimaryVertex",
            FatalException,
            "Cannot record source position: event has no primary vertex"
        );
        return;
    }

    // Save one row per sensor, including sensors with zero photons.
    for (G4int sensorID = 0; sensorID < 100; ++sensorID)  //0-99, change id no. of SiPMs have been changed
    {
        const auto& energies = fPhotonEnergies[sensorID];

        fOutput
            << runID << ','
            << event->GetEventID() << ','
            << sensorID << ','
            << energies.size() << ",\"";

        // Store the energy list in one quoted CSV field.
        for (std::size_t i = 0; i < energies.size(); ++i)
        {
            if (i != 0)
                fOutput << ' ';

            fOutput << energies[i]/eV;
        }

        fOutput << "\"\n";
    }

    //=========================DATA COLLECTION=======================================================================
    // Add together the photon counts from all 16 SiPMs.
    std::size_t collected = 0;

    for (const auto& energies : fPhotonEnergies)
        collected += energies.size();


    // Read the crystal's actual material properties._____________________________________________
    const auto* crystal =
        G4Material::GetMaterial("G4_CESIUM_IODIDE");

    auto* properties =
        crystal ? crystal->GetMaterialPropertiesTable() : nullptr;

    if (!properties ||
        !properties->ConstPropertyExists("SCINTILLATIONYIELD"))
    {
        G4Exception(
            "EventAction::EndOfEventAction",
            "MissingScintillationYield",
            FatalException,
            "Cannot record P1: crystal scintillation yield is missing."
        );
        return;
    }

    auto* absorption = properties->GetProperty("ABSLENGTH");

    if (!absorption || absorption->GetVectorLength() == 0)
    {
        G4Exception(
            "EventAction::EndOfEventAction",
            "MissingAbsorptionLength",
            FatalException,
            "Cannot record P2: crystal absorption length is missing."
        );
        return;
    }

    // A single P2 is valid here only if ABSLENGTH is constant
    // across the tabulated optical-photon energies.
    const auto absorptionLength = (*absorption)[0];

    for (std::size_t i = 1; i < absorption->GetVectorLength(); ++i)
    {
        if (std::abs((*absorption)[i] - absorptionLength)
            > 1e-9 * std::abs(absorptionLength))
        {
            G4Exception(
                "EventAction::EndOfEventAction",
                "EnergyDependentAbsorption",
                FatalException,
                "ABSLENGTH varies with photon energy; cannot record one P2."
            );
            return;
        }
    }

    const auto p1 =
        properties->GetConstProperty("SCINTILLATIONYIELD") * MeV;

    const auto p2 = absorptionLength / cm;
    //===============================================================================================================




    // Write one summary row for this event.
    fSummary
        << runID << ','
        << event->GetEventID() << ','
        << sourceVertex->GetX0()/mm << ','               // (for recording gamma launch coordinates)
        << sourceVertex->GetY0()/mm << ','               // (for recording gamma launch coordinates)
        << sourceVertex->GetZ0()/mm << ','               // (for recording gamma launch coordinates)
        << fGenerated << ','
        << fAbsorbed << ','
        << collected << ','
        << p1 << ','
        << p2 << '\n';

    fSummary.flush();
    //===============================================================================

    // Flush once per event, rather than once per photon.
    fOutput.flush();

    if (!fOutput || !fSummary)
    {
        G4Exception(
            "EventAction::EndOfEventAction",
            "OutputWriteFailed",
            FatalException,
            "Could not write photon data"
        );
    }
}
