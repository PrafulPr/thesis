#include "EventAction.hh"

#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"

#include <iomanip>

EventAction::EventAction()
    : fOutput("photons.csv")
{
    if (!fOutput.is_open())
    {
        G4Exception(
            "EventAction::EventAction",
            "OutputOpenFailed",
            FatalException,
            "Cannot open photons.csv"
        );
    }

    fOutput << std::setprecision(10);

    fOutput
        << "run_id,event_id,sensor_id,photon_count,energies_eV\n";
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    // Empty the lists, retaining their allocated memory.
    for (auto& energies : fPhotonEnergies)
        energies.clear();
}

void EventAction::RecordPhoton(G4int sensorID, G4double energy)
{
    if (sensorID < 0 || sensorID >= 16)
    {
        G4Exception(
            "EventAction::RecordPhoton",
            "InvalidSensorID",
            FatalException,
            "SiPM ID must be between 0 and 15"
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
    for (G4int sensorID = 0; sensorID < 16; ++sensorID)
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

    // Flush once per event, rather than once per photon.
    fOutput.flush();

    if (!fOutput)
    {
        G4Exception(
            "EventAction::EndOfEventAction",
            "OutputWriteFailed",
            FatalException,
            "Could not write photon data"
        );
    }
}
