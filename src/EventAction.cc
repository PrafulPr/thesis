#include "EventAction.hh"

#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"

#include <iomanip>

EventAction::EventAction()
    : fOutput("photons.csv"), fSummary("photon_summary.csv")
{
    if (!fOutput.is_open() || !fSummary.is_open())
    {
        G4Exception(
            "EventAction::EventAction",
            "OutputOpenFailed",
            FatalException,
            "Cannot open photon OUTPUT FILES.csv"
        );
    }

    fSummary << "run_id,event_id,generated,absorbed,collected\n"; //FOR DATA COLLECTION
    fOutput << std::setprecision(10);

    fOutput
        << "run_id,event_id,sensor_id,photon_count,energies_eV\n";
}

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

    //=========================DATA COLLECTION=====================================
    // Add together the photon counts from all 16 SiPMs.
    std::size_t collected = 0;

    for (const auto& energies : fPhotonEnergies)
        collected += energies.size();

    // Write one summary row for this event.
    fSummary
        << runID << ','
        << event->GetEventID() << ','
        << fGenerated << ','
        << fAbsorbed << ','
        << collected << '\n';

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
