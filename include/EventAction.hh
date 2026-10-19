#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <array>
#include <vector>
#include <fstream>

class G4Event;

class EventAction : public G4UserEventAction
{
public:
    EventAction();

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void RecordPhoton(G4int sensorID, G4double energy);

private:
    std::array<std::vector<G4double>, 16> fPhotonEnergies;
    std::ofstream fOutput;
};

#endif