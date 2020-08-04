SET(EXAMPLE_SOURCES

	ODE/PredatorPrey.xml
	ODE/CellCycle.xml
	ODE/CellCycle_Global.xml
	ODE/MAPK_SBML.xml
	ODE/LateralSignaling.xml
	ODE/CellCycleDelay.xml


	PDE/ActivatorInhibitor_1D.xml
	PDE/ActivatorInhibitor_2D.xml
	PDE/ActivatorInhibitor_Domain.xml
	PDE/domain.tif
	PDE/TuringPatterns.xml
	PDE/ExcitableMedium_3D.xml


	CPM/CellSorting_2D.xml
	CPM/CellSorting_3D.xml
	CPM/Proliferation_2D.xml
	CPM/Proliferation_3D.xml
	CPM/Persistence_2D.xml
	CPM/Protrusion_2D.xml
	CPM/Protrusion_3D.xml
	CPM/Crypt.xml
	CPM/crypt.tif
	CPM/RunAndTumble.xml
	CPM/ConvergentExtension.xml
	#CPM/Protrusion.xml
	CPM/CPM_Game_of_Life.xml
	CPM/Stigmergy_VectorField.xml


	Multiscale/AutocrineChemotaxis.xml
	Multiscale/CellCycle.xml
	Multiscale/CellCycle_3D.xml
	Multiscale/CellCycle_PDE.xml
	Multiscale/CellPolarity.xml
	Multiscale/PlanarCellPolarity.xml
	Multiscale/VascularPatterning.xml
	Multiscale/Dictyostelium.xml
	Multiscale/ExcitableTissue.xml


	Miscellaneous/GameOfLife.xml
	Miscellaneous/GameOfLife_Global.xml
	Miscellaneous/FrenchFlag.xml
# 	Miscellaneous/ParticleAggregation.xml
	Miscellaneous/ShellCA.xml

)

IF (NOT MORPHEUS_RELEASE_BUNDLE)
	SET(EXAMPLE_SOURCES
	# add non-public examples
	${EXAMPLE_SOURCES})
ENDIF()

SET(EXAMPLES_SRC ${EXAMPLE_SOURCES} PARENT_SCOPE)
SET(EXAMPLES_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)



