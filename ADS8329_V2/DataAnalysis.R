library(h5)
file = h5file(name = "Data1.bin", mode = "r")
datasets = list.datasets(file)
VectorData = c();
for (dataset in 1:length(datasets)) {
  Data = readDataSet(file[datasets[dataset]])
  for (i in 1:20) {
    VectorData = c(VectorData, Data[i,])
  }
}

