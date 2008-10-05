/** \file gtbary.cxx
    \brief Factory for gtbary application.
    \authors Masaharu Hirayama, GSSC,
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/TimeCorrectorApp.h"

#include "st_app/StAppFactory.h"

st_app::StAppFactory<timeSystem::TimeCorrectorApp> g_factory("gtbary");
