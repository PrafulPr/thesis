#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4TouchableHandle.hh"
#include "G4Exception.hh"

//FOR DATA CLLECTION:
#include "G4VProcess.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4OpBoundaryProcess.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    //=======================DATA COLLECTION=========================================
    // Count optical photons created during this step.
    for (const auto* secondary : *step->GetSecondaryInCurrentStep())
    {
        if (secondary->GetDefinition() ==
            G4OpticalPhoton::OpticalPhotonDefinition())
        {
            fEventAction->RecordGenerated();
        }
    }
    auto track = step->GetTrack();
    //===============================================================================

    // Ignore gamma rays, electrons and all other particle types.
    if (track->GetDefinition() !=
        G4OpticalPhoton::OpticalPhotonDefinition())
        return;

    const auto pre = step->GetPreStepPoint();         //get pre info
    const auto post = step->GetPostStepPoint();       //get post info

//_______________________________________________________________________________________________
//==================== DATA COLLECTION ==========================================================

    // Count absorption inside a material.
    const auto* process = post->GetProcessDefinedStep();

    if (process && process->GetProcessName() == "OpAbsorption")
    {
        fEventAction->RecordAbsorbed();
        return;
    }

    // Count absorption at an optical surface.
    if (post->GetStepStatus() == fGeomBoundary)
    {
        auto* processes =
            track->GetDefinition()->GetProcessManager()->GetProcessList();

        for (G4int i = 0; i < processes->size(); ++i)
        {
            auto* boundary =
                dynamic_cast<G4OpBoundaryProcess*>((*processes)[i]);

            if (boundary && boundary->GetStatus() == Absorption)
            {
                fEventAction->RecordAbsorbed();
                return;
            }
        }
    }
//===============================================================================================
//_______________________________________________________________________________________________

    // ================= ENSURE geometrical boundaries: =======================
    if (post->GetStepStatus() != fGeomBoundary)
        return;

    const auto preVolume = pre->GetPhysicalVolume();
    const auto postVolume = post->GetPhysicalVolume();

    if (!preVolume || !postVolume)
        return;
    // ========================================================================

    // FIND CRYSTAL AND SENSOR volumes and saves pointers to them so as to ise later
    if (!fCrystalLogical || !fSensorLogical)
    {
        auto store = G4LogicalVolumeStore::GetInstance();

        fCrystalLogical = store->GetVolume("Crystal", false);
        fSensorLogical = store->GetVolume("SiPM", false);

        if (!fCrystalLogical || !fSensorLogical)
        {
            G4Exception(
                "SteppingAction::UserSteppingAction",
                "MissingDetectorVolume",
                FatalException,
                "Cannot find Crystal or SiPM logical volume"
            );
            return;
        }
    }

    if (preVolume->GetLogicalVolume() != fCrystalLogical ||  //ensures ONLY CRYSTAL TO SiPM
        postVolume->GetLogicalVolume() != fSensorLogical)
        return;

    const auto sensorID =
        post->GetTouchableHandle()->GetCopyNumber();

    const auto energy = pre->GetKineticEnergy();

    // Record first, then stop the photon.
    fEventAction->RecordPhoton(sensorID, energy);
    track->SetTrackStatus(fStopAndKill);   //KIlls photon
}