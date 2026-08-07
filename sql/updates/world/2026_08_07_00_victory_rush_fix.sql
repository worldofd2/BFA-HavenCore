DELETE FROM spell_proc WHERE SpellId = 32215;

INSERT INTO spell_proc (
    SpellId,
    SchoolMask,
    SpellFamilyName,
    SpellFamilyMask0,
    SpellFamilyMask1,
    SpellFamilyMask2,
    SpellFamilyMask3,
    ProcFlags,
    SpellTypeMask,
    SpellPhaseMask,
    HitMask,
    AttributesMask,
    DisableEffectsMask,
    ProcsPerMinute,
    Chance,
    Cooldown,
    Charges
)
VALUES (
    32215,      -- Victorious
    0,          -- SchoolMask
    4,          -- SPELLFAMILY_WARRIOR
    0,          -- SpellFamilyMask0
    0,          -- SpellFamilyMask1
    0,          -- SpellFamilyMask2
    0,          -- SpellFamilyMask3
    0x00000002, -- PROC_FLAG_KILL
    0,          -- SpellTypeMask
    0,          -- SpellPhaseMask
    0,          -- HitMask
    0,          -- AttributesMask
    0,          -- DisableEffectsMask
    0,          -- ProcsPerMinute
    100,        -- 100% proc chance
    0,          -- Cooldown
    0           -- Charges
);