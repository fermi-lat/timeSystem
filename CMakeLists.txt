##### Library ######
add_library(
  timeSystem STATIC
  src/AbsoluteTime.cxx
  src/axBary.c
  src/bary.c
  src/BaryTimeComputer.cxx
  src/CalendarFormat.cxx
  src/clock.c
  src/ctatv.c
  src/dpleph.c
  src/Duration.cxx
  src/ElapsedTime.cxx
  src/EventTimeHandler.cxx
  src/glastscorbit.c
  src/GlastTimeHandler.cxx
  src/IntFracUtility.cxx
  src/MjdFormat.cxx
  src/phaseHist.c
  src/PulsarTestApp.cxx
  src/scorbit.c
  src/SourcePosition.cxx
  src/TimeCorrectorApp.cxx
  src/TimeInterval.cxx
  src/TimeSystem.cxx
  src/xCC.c
)

target_link_libraries(
  timeSystem
  PUBLIC PkgConfig::cfitsio st_app st_stream st_facilities tip
)

target_include_directories(
  timeSystem PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

###### Executables ######
add_executable(gtbary src/gtbary/gtbary.cxx)
target_link_libraries(gtbary PRIVATE timeSystem)

###### Tests ######
add_executable(test_timeSystem src/test/test_timeSystem.cxx)
target_link_libraries(test_timeSystem PRIVATE timeSystem)

###############################################################
# Installation
###############################################################
install(DIRECTORY timeSystem DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY data/ DESTINATION ${FERMI_INSTALL_DATADIR})
install(DIRECTORY pfiles/ DESTINATION ${FERMI_INSTALL_PFILESDIR})

install(
  TARGETS timeSystem gtbary
  # EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
