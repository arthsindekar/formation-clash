#include "MyGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Combat/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Containers/Queue.h"


void AMyGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    
    TArray<AActor*> AllUnits;
    UWorld* CurrentWorld = GetWorld();
    UGameplayStatics::GetAllActorsOfClass(CurrentWorld, ACharacter::StaticClass(), AllUnits);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("Found %d units"), AllUnits.Num()));

    for (AActor* Unit : AllUnits)
    {
        UCombatComponent* Combat = Unit->FindComponentByClass<UCombatComponent>();
        if (Combat)
        {
            Combat->OnUnitDeath.AddDynamic(this, &AMyGameModeBase::OnUnitDead);
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                    FString::Printf(TEXT("Bound death event for: %s"), *Unit->GetName()));
        }
    }
    
    PopulateLevelMap();
}

void AMyGameModeBase::PopulateLevelMap()
{
    FString CurrentMap = GetWorld()->GetMapName();
    CurrentMap.RemoveFromStart("UEDPIE_0_"); // strip editor prefix
    
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("Current map: %s"), *CurrentMap));
    
    for (int32 i = 0; i < Levels.Num(); i++)
    {
        if (Levels[i].NameOfLevel.ToString() == CurrentMap)
        {
            LevelTrack = i;
            CurrentLevel = Levels[i]; 
            break;
        }
    }
}

void AMyGameModeBase::OnUnitDead()
{
    
    
    if (!bBattleOver)
    {
        IsBattleOver();
    }
}



void AMyGameModeBase::IsBattleOver()
{
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllUnits);

    bool bTeam0Alive = false;
    bool bTeam1Alive = false;

    for (AActor* Unit : AllUnits)
    {
        UCombatComponent* Combat = Unit->FindComponentByClass<UCombatComponent>();
        if (Combat && !Combat->bIsDead)
        {
            if (Combat->TeamID == 0) bTeam0Alive = true;
            if (Combat->TeamID == 1) bTeam1Alive = true;
        }
    }
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
            FString::Printf(TEXT("Team0 alive: %d | Team1 alive: %d | WidgetClass: %d"),
                bTeam0Alive, bTeam1Alive, ResultWidgetClass != nullptr));


    if (!bTeam0Alive || !bTeam1Alive)
    {
        bBattleOver = true;

        if (ResultWidgetClass)
        {
            UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), ResultWidgetClass);
            if (Widget)
            {
                // Set the result text
                UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("ResultText")));
                if (Text)
                {
                    if (!bTeam0Alive)
                    {
                        // Lost — replay same level after 2 seconds
                        Text->SetText(FText::FromString("GAME OVER"));
                        FTimerHandle Handle;
                        GetWorld()->GetTimerManager().SetTimer(Handle, [this]()
                        {
                            UGameplayStatics::OpenLevel(GetWorld(), Levels[LevelTrack].NameOfLevel);
                        }, 2.0f, false);
                    }
                    else
                    {
                        // Won — next level after 2 seconds
                        Text->SetText(FText::FromString("LEVEL CLEARED"));
                        FTimerHandle Handle;
                        GetWorld()->GetTimerManager().SetTimer(Handle, [this]()
                        {
                            LevelTrack++;
                            UGameplayStatics::OpenLevel(GetWorld(), Levels[LevelTrack].NameOfLevel);
                        }, 2.0f, false);
                    }
                }

                Widget->AddToViewport();
            }
        }

        // Pause the game
        //UGameplayStatics::SetGamePaused(GetWorld(), true);
    }
}

void AMyGameModeBase::OnAllUnitsDead()
{
    //Get All Actors and clear them once all enemies are dead ahead of new level
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllUnits);
    for (AActor* Unit : AllUnits)
    {
        UCombatComponent* UnitCombatComponent = Unit->FindComponentByClass<UCombatComponent>();
        if (UnitCombatComponent->TeamID == 1)
        {
            Unit->Destroy();
        }
    }
    
}

void AMyGameModeBase::SpawnEnemies(FLevelData Level)
{
    // For all spawn info in current level
    for (FEnemySpawnInfo& SpawnInfo : Level.EnemyLocs)
    {
        ACharacter* SpawnedEnemy = GetWorld()->SpawnActor<ACharacter>(
            SpawnInfo.EnemyClass,SpawnInfo.SpawnLocation,FRotator::ZeroRotator);
        //Assign the newly spawned enemy its ID
        SpawnedEnemy->FindComponentByClass<UCombatComponent>()->TeamID = 1;
        SpawnedEnemy->FindComponentByClass<UCombatComponent>()->OnUnitDeath.AddDynamic(this, 
            &AMyGameModeBase::OnUnitDead);
    }
    
}

void AMyGameModeBase::NextLevel()
{
    
    if (Levels.IsEmpty())
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("YOU WIN! ALL LEVELS COMPLETE"));
        return;
    }
    
    LevelQueue.Dequeue(CurrentLevel);
    bBattleOver = false;
    //Clear field and move on to next level
    OnAllUnitsDead();
    SpawnEnemies(CurrentLevel);
}
