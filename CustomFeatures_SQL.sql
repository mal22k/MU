-- ============================================
-- TABLAS PARA CUSTOM FEATURES - MU ONLINE
-- ============================================

-- ============================================
-- Battle Pass Table
-- ============================================
CREATE TABLE CustomBattlePass (
    AccountID VARCHAR(11) NOT NULL,
    CharName VARCHAR(11) NOT NULL,
    BPLevel TINYINT DEFAULT 0,
    BPExp INT DEFAULT 0,
    SeasonId TINYINT DEFAULT 1,
    ClaimedFree VARBINARY(100) DEFAULT 0x0,
    ClaimedVip1 VARBINARY(100) DEFAULT 0x0,
    ClaimedVip2 VARBINARY(100) DEFAULT 0x0,
    ClaimedVip3 VARBINARY(100) DEFAULT 0x0,
    PRIMARY KEY (AccountID, CharName)
);

-- ============================================
-- Extended Master Skill Tree Table
-- ============================================
CREATE TABLE CustomExtMasterTree (
    CharName VARCHAR(11) NOT NULL PRIMARY KEY,
    TotalPoints SMALLINT DEFAULT 0,
    UsedPoints SMALLINT DEFAULT 0,
    SkillLevels VARBINARY(50) DEFAULT 0x0
);

-- ============================================
-- Nota: Kill Streak NO necesita tabla
-- Solo se trackea en memoria durante la sesión del jugador
-- ============================================
