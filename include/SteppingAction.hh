#ifndef STEPPING_ACTION_HH
#define STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

class EventAction;
class G4Step;
class G4LogicalVolume;

class SteppingAction : public G4UserSteppingAction
{
public:
    explicit SteppingAction(EventAction* eventAction);

    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* fEventAction;

    G4LogicalVolume* fCrystalLogical = nullptr;
    G4LogicalVolume* fSensorLogical = nullptr;
};

#endif